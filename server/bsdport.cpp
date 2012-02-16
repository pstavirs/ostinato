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
#include <net/route.h>

#ifdef Q_OS_MAC
#define ifr_flagshigh ifr_flags
#define IFF_PPROMISC (IFF_PROMISC << 16)
#endif

QList<BsdPort*> BsdPort::allPorts_;
BsdPort::StatsMonitor *BsdPort::monitor_;

BsdPort::BsdPort(int id, const char *device)
    : PcapPort(id, device) 
{
    isPromisc_ = true;
    clearPromisc_ = false;

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

void BsdPort::init()
{
    if (!monitor_->isRunning())
        monitor_->start();

    monitor_->waitForSetupFinished();

    if (!isPromisc_)
        addNote("Non Promiscuous Mode");
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

                Q_ASSERT(state);
#ifdef Q_OS_MAC
                *state = ifm->ifm_flags & IFF_RUNNING ? 
                    OstProto::LinkStateUp : OstProto::LinkStateDown;
#else
                *state = (OstProto::LinkState) ifd->ifi_link_state;
#endif

                stats->rxPps  = (ifd->ifi_ipackets + ifd->ifi_noproto 
                                    - stats->rxPkts) /kRefreshFreq_;
                stats->rxBps  = (ifd->ifi_ibytes - stats->rxBytes)
                                    /kRefreshFreq_;
                stats->rxPkts  = ifd->ifi_ipackets + ifd->ifi_noproto;
                stats->rxBytes = ifd->ifi_ibytes;
                stats->txPps  = (ifd->ifi_opackets - stats->txPkts)
                                    /kRefreshFreq_;
                stats->txBps  = (ifd->ifi_obytes - stats->txBytes)
                                    /kRefreshFreq_;
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
