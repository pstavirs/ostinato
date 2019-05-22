/*
Copyright (C) 2012 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "bsdport.h"

#include "interfaceinfo.h"

#ifdef Q_OS_BSD4

#include <QByteArray>
#include <QHash>
#include <QTime>

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/route.h>
#include <unistd.h>

#ifdef Q_OS_MAC
#define ifr_flagshigh ifr_flags
#define IFF_PPROMISC (IFF_PROMISC << 16)
#endif

#ifndef SA_SIZE // For some reason MacOS doesn't define this while BSD does
// And the story of how to roundup is ugly - see
// https://github.com/FRRouting/frr/blob/master/zebra/kernel_socket.c
#ifdef __APPLE__
#define ROUNDUP_TYPE int
#else
#define ROUNDUP_TYPE long
#endif
#define SA_SIZE(sa)                                             \
    (  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?      \
        sizeof(ROUNDUP_TYPE)            :                               \
        1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(ROUNDUP_TYPE) - 1) ) )
#endif

struct ifaddrs *BsdPort::addressList_{nullptr};
QByteArray BsdPort::routeListBuffer_;

QList<BsdPort*> BsdPort::allPorts_;
BsdPort::StatsMonitor *BsdPort::monitor_;

const quint32 kMaxValue32 = 0xffffffff;

BsdPort::BsdPort(int id, const char *device)
    : PcapPort(id, device) 
{
    isPromisc_ = true;
    clearPromisc_ = false;
    ifIndex_ = if_nametoindex(device);

    populateInterfaceInfo();

    // We don't need per port Rx/Tx monitors for Bsd
    delete monitorRx_;
    delete monitorTx_;
    monitorRx_ = monitorTx_ = NULL;

    // We have one monitor for both Rx/Tx of all ports
    if (!monitor_)
        monitor_ = new StatsMonitor();

    data_.set_is_exclusive_control(hasExclusiveControl());
    minPacketSetSize_ = 16;

    qDebug("adding dev to all ports list <%s>", device);
    allPorts_.append(this);

    maxStatsValue_ = ULONG_MAX;
}

BsdPort::~BsdPort()
{
    qDebug("In %s", __FUNCTION__);

    if (monitor_->isRunning())
    {
        monitor_->stop();
        monitor_->wait();
    }

    if (clearPromisc_)
    {
        int sd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, name(), sizeof(ifr.ifr_name));

        if (ioctl(sd, SIOCGIFFLAGS, &ifr) != -1) 
        {
            short promisc = IFF_PPROMISC >> 16;

            if (ifr.ifr_flagshigh & promisc)
            {
                ifr.ifr_flagshigh &= ~promisc;
                if (ioctl(sd, SIOCSIFFLAGS, &ifr) == -1)
                    qDebug("Failed clearing promisc flag. SIOCSIFFLAGS failed: %s",
                        strerror(errno));
                else
                    qDebug("Cleared promisc successfully");
            }
            else
                qDebug("clear_promisc is set but IFF_PPROMISC is not?");
        }
        else
            qDebug("Failed clearing promisc flag. SIOCGIFFLAGS failed: %s",
                    strerror(errno));

        close(sd);
    }
}

void BsdPort::fetchHostNetworkInfo()
{
    if (getifaddrs(&addressList_) < 0)
    {
        qWarning("getifaddrs() failed: %s", strerror(errno));
        return;
    }

    size_t len;
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_UNSPEC, NET_RT_FLAGS, RTF_GATEWAY};
    if (sysctl(mib, sizeof(mib)/sizeof(int), 0, &len, 0, 0) < 0)
    {
        qWarning("sysctl CTL_NET|PF_ROUTE failed fetching buflen: %s", strerror(errno));
        return;
    }

    routeListBuffer_.resize(len); 
    if (sysctl(mib, sizeof(mib)/sizeof(int), routeListBuffer_.data(), &len, 0, 0) < 0)
    {
        qWarning("sysctl CTL_NET|PF_ROUTE failed: %s", strerror(errno));
        return;
    }
}

void BsdPort::freeHostNetworkInfo()
{
    freeifaddrs(addressList_);
    addressList_ = nullptr;

    routeListBuffer_.resize(0); // release allocated memory
}

void BsdPort::init()
{
    if (!monitor_->isRunning())
        monitor_->start();

    monitor_->waitForSetupFinished();

    if (!isPromisc_)
        addNote("Non Promiscuous Mode");

    AbstractPort::init();
}

bool BsdPort::hasExclusiveControl() 
{
    // TODO
    return false;
}

bool BsdPort::setExclusiveControl(bool /*exclusive*/) 
{
    // TODO
    return false;
}

void BsdPort::populateInterfaceInfo()
{
    //
    // Find Mac
    //
    quint64 mac = 0;
    struct ifaddrs *addr;
    for (addr = addressList_; addr != NULL; addr = addr->ifa_next)
    {
        if (strcmp(addr->ifa_name, name()) == 0)
        {
            if (addr->ifa_addr->sa_family == AF_LINK)
            {
                mac = qFromBigEndian<quint64>(
                        LLADDR((struct sockaddr_dl *)(addr->ifa_addr))) >> 16;
                break;
            }
        }
    }

    interfaceInfo_ = new InterfaceInfo;
    interfaceInfo_->mac = mac;

    //
    // Find gateways
    //
    static_assert(RTA_DST == 0x1, "RTA_DST is not 0x1"); // Validate assumption
    static_assert(RTA_GATEWAY == 0x2, "RTA_GATEWAY is not 0x2"); // Validate assumption
    quint32 gw4 = 0;
    UInt128 gw6 = 0;
    const char *p = routeListBuffer_.constData();
    const char *end = p + routeListBuffer_.size();
    while (!gw4 || !gw6)
    {
        const struct rt_msghdr *rt = (const struct rt_msghdr*) p;
        const struct sockaddr *sa = (const struct sockaddr*)(rt + 1); // RTA_DST = 0x1
        if ((rt->rtm_index == ifIndex_)
                && ((rt->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY))) 
        {
            if (!gw4 && sa->sa_family == AF_INET) 
            {
                if (((sockaddr_in*)sa)->sin_addr.s_addr == 0) // default route 0.0.0.0
                {
                    sa = (struct sockaddr *)((char *)sa + SA_SIZE(sa)); 
                    gw4 = qFromBigEndian<quint32>(
                            ((sockaddr_in*)sa)->sin_addr.s_addr); // RTA_GW = 0x2
                }
            }
            if (!gw6 && sa->sa_family == AF_INET6)
            {
                if (UInt128((quint8*)(((sockaddr_in6*)sa)->sin6_addr.s6_addr))
                        == UInt128(0,0)) // default route ::
                {
                    sa = (struct sockaddr *)((char *)sa + SA_SIZE(sa)); 
                    gw6 = UInt128((quint8*)(
                            ((sockaddr_in6*)sa)->sin6_addr.s6_addr)); // RTA_GW = 0x2
                }
            }
        }
        p += rt->rtm_msglen;
        if (p >= end)
            break;
    }

    //
    // Find self IP
    //
    addr = addressList_;
    while (addr)
    {
        if (strcmp(addr->ifa_name, name()) == 0)
        {
            if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_INET)
            {
                Ip4Config ip;
                ip.address = qFromBigEndian<quint32>(
                        ((struct sockaddr_in *)(addr->ifa_addr))->sin_addr.s_addr);
                ip.prefixLength = std::bitset<32>( 
                            ((struct sockaddr_in *)(addr->ifa_netmask))->sin_addr.s_addr)
                        .count();
                ip.gateway = gw4;
                interfaceInfo_->ip4.append(ip);
            }
            else if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_INET6)
            {
                Ip6Config ip;
                ip.address = UInt128((quint8*)
                        ((struct sockaddr_in6 *)(addr->ifa_addr))->sin6_addr.s6_addr);
                Q_ASSERT(addr->ifa_netmask);
                ip.prefixLength = std::bitset<64>(qFromBigEndian<quint64>(
                            ((struct sockaddr_in6 *)(addr->ifa_netmask))
                                ->sin6_addr.s6_addr))
                        .count();
                ip.prefixLength += std::bitset<64>(qFromBigEndian<quint64>(
                            ((struct sockaddr_in6 *)(addr->ifa_netmask))
                                ->sin6_addr.s6_addr+8))
                        .count();
                ip.gateway = gw6;
                interfaceInfo_->ip6.append(ip);
            }
        }
        addr = addr->ifa_next;
    }
}

BsdPort::StatsMonitor::StatsMonitor()
    : QThread()
{
    stop_ = false;
    setupDone_ = false;
}

void BsdPort::StatsMonitor::run()
{
    int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
    const int mibLen = sizeof(mib)/sizeof(mib[0]);
    QHash<uint, PortStats*> portStats;
    QHash<uint, OstProto::LinkState*> linkState;
    int sd;
    QByteArray buf;
    size_t len;
    char *p, *end;
    int count;
    struct ifreq ifr;

    //
    // We first setup stuff before we start polling for stats
    //
    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) 
    {
        qWarning("sysctl NET_RT_IFLIST(1) failed (%s)\n", strerror(errno));
        return;
    } 

    qDebug("sysctl mib returns reqd len = %d\n", (int) len);
    len *= 2; // for extra room, just in case!
    buf.fill('\0', len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) 
    {
        qWarning("sysctl NET_RT_IFLIST(2) failed(%s)\n", strerror(errno));
        return;
    } 

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    Q_ASSERT(sd >= 0);
    memset(&ifr, 0, sizeof(ifr));

    //
    // Populate the port stats hash table
    //
    p = buf.data();
    end = p + len;
    count = 0;
    while (p < end) 
    {
        struct if_msghdr *ifm = (struct if_msghdr*) p;
        struct sockaddr_dl *sdl = (struct sockaddr_dl*) (ifm + 1);

        if (ifm->ifm_type == RTM_IFINFO)
        {
            char ifname[1024];

            strncpy(ifname, sdl->sdl_data, sdl->sdl_nlen);
            ifname[sdl->sdl_nlen] = 0;

            qDebug("if: %s(%d, %d)", ifname, ifm->ifm_index, sdl->sdl_index);
            foreach(BsdPort* port, allPorts_)
            {
                if (strncmp(port->name(), sdl->sdl_data, sdl->sdl_nlen) == 0)
                {
                    Q_ASSERT(ifm->ifm_index == sdl->sdl_index);
                    portStats[uint(ifm->ifm_index)] = &(port->stats_);
                    linkState[uint(ifm->ifm_index)] = &(port->linkState_);

                    // Set promisc mode, if not already set
                    strncpy(ifr.ifr_name, port->name(), sizeof(ifr.ifr_name));
                    if (ioctl(sd, SIOCGIFFLAGS, &ifr) != -1)
                    {
                        short promisc = IFF_PPROMISC >> 16;

                        if ((ifr.ifr_flagshigh & promisc) == 0)
                        {
                            ifr.ifr_flagshigh |= promisc;
                            if (ioctl(sd, SIOCSIFFLAGS, &ifr) != -1)
                            {
                                qDebug("%s: set promisc successful", 
                                        port->name());
                                port->clearPromisc_ = true;
                            }
                            else
                            {
                                port->isPromisc_ = false;
                                qDebug("%s: failed to set promisc; "
                                        "SIOCSIFFLAGS failed (%s)", 
                                        port->name(), strerror(errno));
                            }
                        }
                        else
                            qDebug("%s: promisc already set", port->name());
                    }
                    else
                    {
                        port->isPromisc_ = false;
                        qDebug("%s: failed to set promisc; SIOCGIFFLAGS failed (%s)",
                                port->name(), strerror(errno));
                    }
                    break;
                }
            }
            count++;
        }
        p += ifm->ifm_msglen;
    }

    qDebug("port count = %d\n", count);
    if (count <= 0)
    {
        qWarning("no ports in NET_RT_IFLIST - no stats will be available");
        return;
    }

    close(sd);

    qDebug("stats for %d ports setup", count);
    setupDone_ = true;

    //
    // We are all set - Let's start polling for stats!
    //
    while (!stop_)
    {
        if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) 
        {
            qWarning("sysctl NET_RT_IFLIST(3) failed(%s)\n", strerror(errno));
            goto _try_later;
        } 

        p = buf.data();
        end = p + len;

        while (p < end)
        {
            struct if_msghdr *ifm = (struct if_msghdr*) p;
            AbstractPort::PortStats *stats;

            if (ifm->ifm_type != RTM_IFINFO)
                goto _next;

            stats = portStats[ifm->ifm_index];
            if (stats)
            {
                struct if_data *ifd = &(ifm->ifm_data);
                OstProto::LinkState *state = linkState[ifm->ifm_index];
                u_long in_packets;

                Q_ASSERT(state);
#ifdef Q_OS_MAC
                *state = ifm->ifm_flags & IFF_RUNNING ? 
                    OstProto::LinkStateUp : OstProto::LinkStateDown;
#else
                *state = (OstProto::LinkState) ifd->ifi_link_state;
#endif

                in_packets = ifd->ifi_ipackets + ifd->ifi_noproto;
                stats->rxPps = 
                    ((in_packets >= stats->rxPkts) ?
                         in_packets - stats->rxPkts :
                         in_packets + (kMaxValue32 - stats->rxPkts))
                     / kRefreshFreq_;
                stats->rxBps  = 
                    ((ifd->ifi_ibytes >= stats->rxBytes) ?
                         ifd->ifi_ibytes - stats->rxBytes :
                         ifd->ifi_ibytes + (kMaxValue32 - stats->rxBytes))
                     / kRefreshFreq_;
                stats->rxPkts  = in_packets;
                stats->rxBytes = ifd->ifi_ibytes;
                stats->txPps  = 
                    ((ifd->ifi_opackets >= stats->txPkts) ?
                         ifd->ifi_opackets - stats->txPkts :
                         ifd->ifi_opackets + (kMaxValue32 - stats->txPkts))
                     / kRefreshFreq_;
                stats->txBps  = 
                    ((ifd->ifi_obytes >= stats->txBytes) ?
                         ifd->ifi_obytes - stats->txBytes :
                         ifd->ifi_obytes + (kMaxValue32 - stats->txBytes))
                     / kRefreshFreq_;
                stats->txPkts  = ifd->ifi_opackets;
                stats->txBytes = ifd->ifi_obytes;

                stats->rxDrops = ifd->ifi_iqdrops;
                stats->rxErrors = ifd->ifi_ierrors;
            }
_next:
            p += ifm->ifm_msglen;
        }
_try_later:
        QThread::sleep(kRefreshFreq_);
    }

    portStats.clear();
    linkState.clear();
}

void BsdPort::StatsMonitor::stop()
{
    stop_ = true;
}

bool BsdPort::StatsMonitor::waitForSetupFinished(int msecs)
{
    QTime t;
   
    t.start();
    while (!setupDone_)
    {
        if (t.elapsed() > msecs)
            return false;

        QThread::msleep(10);
    }

    return true;
}
#endif
