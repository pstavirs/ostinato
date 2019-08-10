/*
Copyright (C) 2011 Srivats P.

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

#include "linuxport.h"

#include "interfaceinfo.h"

#ifdef Q_OS_LINUX

#include "../common/qtport.h"

#include <QByteArray>
#include <QHash>
#include <QTime>

#include <errno.h>
#include <fcntl.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#if (LIBNL_VER_NUM > 0x0302) || ((LIBNL_VER_NUM == 0x0302) && (LIBNL_VER_MIC >= 26))
#include <net/if.h>
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/rtnetlink.h>

QList<LinuxPort*> LinuxPort::allPorts_;
LinuxPort::StatsMonitor *LinuxPort::monitor_;

const quint32 kMaxValue32 = 0xffffffff;
const quint64 kMaxValue64 = 0xffffffffffffffffULL;

#ifdef HAVE_IFLA_STATS64
#define X_IFLA_STATS IFLA_STATS64
typedef struct rtnl_link_stats64 x_rtnl_link_stats;
#else
#define X_IFLA_STATS IFLA_STATS
typedef struct rtnl_link_stats x_rtnl_link_stats;
#endif

nl_sock  *LinuxPort::netSock_{nullptr};
nl_cache *LinuxPort::linkCache_{nullptr};
nl_cache *LinuxPort::addressCache_{nullptr};
nl_cache *LinuxPort::routeCache_{nullptr};

LinuxPort::LinuxPort(int id, const char *device)
    : PcapPort(id, device) 
{
    isPromisc_ = true;
    clearPromisc_ = false;

    populateInterfaceInfo();

    // We don't need per port Rx/Tx monitors for Linux
    // No need to stop them because we start them only in
    // PcapPort::init which has not yet been called
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

    // A port can support either 32 or 64 bit stats - we will attempt
    // to guess this for each port and initialize this variable at 
    // run time when the counter wraps around
    maxStatsValue_ = 0;
}

LinuxPort::~LinuxPort()
{
    qDebug("In %s", __FUNCTION__);

    allPorts_.removeAll(this);

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
            if (ifr.ifr_flags & IFF_PROMISC)
            {
                ifr.ifr_flags &= ~IFF_PROMISC;
                if (ioctl(sd, SIOCSIFFLAGS, &ifr) == -1)
                    qDebug("Failed clearing promisc flag. SIOCSIFFLAGS failed: %s",
                        strerror(errno));
            }
        }
        else
            qDebug("Failed clearing promisc flag. SIOCGIFFLAGS failed: %s",
                    strerror(errno));

        close(sd);
    }
}

void LinuxPort::fetchHostNetworkInfo()
{
    netSock_ = nl_socket_alloc();
    if (!netSock_) {
        qWarning("Failed to open netlink socket");
        return;
    }

    if (nl_connect(netSock_, NETLINK_ROUTE) < 0) {
        qWarning("Failed to connect netlink socket");
        return;
    }

    if (rtnl_link_alloc_cache(netSock_, AF_UNSPEC, &linkCache_) < 0) {
        qWarning("Failed to populate link cache");
        return;
    }

    if (rtnl_addr_alloc_cache(netSock_, &addressCache_) < 0) {
        qWarning("Failed to populate addr cache");
        return;
    }

    if (rtnl_route_alloc_cache(netSock_, AF_UNSPEC, 0, &routeCache_) < 0) {
        qWarning("Failed to populate addr cache");
        return;
    }
}

void LinuxPort::freeHostNetworkInfo()
{
    nl_cache_put(routeCache_);
    nl_cache_put(addressCache_);
    nl_cache_put(linkCache_);
    nl_socket_free(netSock_);
}

void LinuxPort::init()
{
    if (!monitor_->isRunning())
        monitor_->start();

    monitor_->waitForSetupFinished();

    if (!isPromisc_)
        addNote("Non Promiscuous Mode");

    AbstractPort::init();
}

OstProto::LinkState LinuxPort::linkState()
{
    return linkState_; 
}

bool LinuxPort::hasExclusiveControl() 
{
    // TODO
    return false;
}

bool LinuxPort::setExclusiveControl(bool /*exclusive*/) 
{
    // TODO
    return false;
}

void LinuxPort::populateInterfaceInfo()
{
    //
    // Find Mac
    //
    if (!linkCache_) {
        qWarning("rtnetlink link cache empty for %s", name());
        return;
    }

    rtnl_link *link = rtnl_link_get_by_name(linkCache_, name());
    if (!link) {
        qWarning("rtnetlink link not found for %s", name());
        return;
    }

    nl_addr *addr = rtnl_link_get_addr(link);
    if (nl_addr_get_family(addr) != AF_LLC) {
        qWarning("unexpected mac family found for %s:%d",
                name(), nl_addr_get_family(addr));
        rtnl_link_put(link);
        return;
    }

    if (nl_addr_get_prefixlen(addr) != 48) {
        qWarning("unexpected mac length for %s:%d",
                name(), nl_addr_get_prefixlen(addr));
        rtnl_link_put(link);
        return;
    }

    quint64 mac = qFromBigEndian<quint64>(nl_addr_get_binary_addr(addr)) >> 16;
    if (!mac) {
        qWarning("zero mac for %s - skipping", name());
        rtnl_link_put(link);
        return;
    }

    int ifIndex = rtnl_link_get_ifindex(link);
    rtnl_link_put(link);

    interfaceInfo_ = new InterfaceInfo;
    interfaceInfo_->mac = mac;

    //
    // Find gateways
    //
    quint32 gw4 = 0;
    UInt128 gw6 = 0;
    for (rtnl_route *rt = routeCache_ ? (rtnl_route*) nl_cache_get_first(routeCache_) : 0;
            rt && (!gw4 || !gw6);
            rt = (rtnl_route*) nl_cache_get_next(OBJ_CAST(rt))) {
        if (rtnl_route_get_table(rt) != RT_TABLE_MAIN) // we want only main RTT
            continue;

        nl_addr *pfx = rtnl_route_get_dst(rt);
        if (nl_addr_get_len(pfx)) // default route has len = 0
            continue;

        if (!rtnl_route_get_nnexthops(rt)) // at least one nh is required
            continue;

        rtnl_nexthop *nh = rtnl_route_nexthop_n(rt, 0);
        if (rtnl_route_nh_get_ifindex(nh) != ifIndex) // ignore gw on other links
            continue;

        if (!gw4 && rtnl_route_get_family(rt) == AF_INET) {
            gw4 = qFromBigEndian<quint32>(
                    nl_addr_get_binary_addr(rtnl_route_nh_get_gateway(nh)));
        }
        else if (!gw6 && rtnl_route_get_family(rt) == AF_INET6) {
            gw6 = UInt128((quint8*)
                    nl_addr_get_binary_addr(rtnl_route_nh_get_gateway(nh)));
        }
    }

    //
    // Find self IP
    //
    if (!addressCache_) {
        qWarning("rtnetlink address cache empty for %s", name());
        return;
    }
    rtnl_addr *l3addr = (rtnl_addr*) nl_cache_get_first(addressCache_);
    while (l3addr) {
        if (rtnl_addr_get_ifindex(l3addr) == ifIndex) {
            if (rtnl_addr_get_family(l3addr) == AF_INET) {
                Ip4Config ip;
                ip.address = qFromBigEndian<quint32>(
                                nl_addr_get_binary_addr(
                                    rtnl_addr_get_local(l3addr)));
                ip.prefixLength = rtnl_addr_get_prefixlen(l3addr);
                ip.gateway = gw4;
                interfaceInfo_->ip4.append(ip);
            }
            else if (rtnl_addr_get_family(l3addr) == AF_INET6) {
                Ip6Config ip;
                ip.address = UInt128((quint8*)nl_addr_get_binary_addr(
                                                    rtnl_addr_get_local(l3addr)));
                ip.prefixLength = rtnl_addr_get_prefixlen(l3addr);
                ip.gateway = gw6;
                interfaceInfo_->ip6.append(ip);
            }
        }
        l3addr = (rtnl_addr*) nl_cache_get_next((nl_object*)l3addr);
    }
}

LinuxPort::StatsMonitor::StatsMonitor()
    : QThread()
{
    stop_ = false;
    setupDone_ = false;
    ioctlSocket_ = socket(AF_INET, SOCK_DGRAM, 0);
    Q_ASSERT(ioctlSocket_ >= 0);
}

LinuxPort::StatsMonitor::~StatsMonitor()
{
    close(ioctlSocket_);
}

void LinuxPort::StatsMonitor::run()
{
    if (netlinkStats() < 0)
    {
        qDebug("netlink stats not available - using /proc stats");
        procStats();
    }
}

void LinuxPort::StatsMonitor::procStats()
{
    PortStats **portStats;
    int fd;
    QByteArray buf;
    int len;
    char *p, *end;
    int count, index;
    const char* fmtopt[] = {
        "%llu%llu%llu%llu%llu%llu%u%u%llu%llu%u%u%u%u%u%u\n",
        "%llu%llu%llu%llu%llu%llu%n%n%llu%llu%u%u%u%u%u%n\n",
    };
    const char *fmt;

    //
    // We first setup stuff before we start polling for stats
    //
    fd = open("/proc/net/dev", O_RDONLY);
    if (fd < 0)
    {
        qWarning("Unable to open /proc/net/dev - no stats will be available");
        return;
    }

    buf.fill('\0', 8192);
    len = read(fd, (void*) buf.data(), buf.size());
    if (len < 0)
    {
        qWarning("initial buffer size is too small. no stats will be available");
        return;
    }

    p = buf.data();
    end = p + len;

    // Select scanf format
    if (strstr(buf, "compressed"))
        fmt = fmtopt[0];
    else 
        fmt = fmtopt[1];

    // Count number of lines - number of ports is 2 less than number of lines
    count = 0;
    while (p < end)
    {
        if (*p == '\n')
            count++;
        p++;
    }
    count -= 2;

    if (count <= 0)
    {
        qWarning("no ports in /proc/dev/net - no stats will be available");
        return;
    }

    portStats = (PortStats**) calloc(count, sizeof(PortStats));
    Q_ASSERT(portStats != NULL);

    //
    // Populate the port stats array
    //
    p = buf.data();

    // Skip first two lines
    while (*p != '\n')
        p++;
    p++;
    while (*p != '\n')
        p++;
    p++;

    index = 0;
    while (p < end)
    {
        char* q;

        // Skip whitespace
        while ((p < end) && (*p == ' '))
            p++;

        q = p;

        // Get interface name
        while ((q < end) && (*q != ':') && (*q != '\n'))
            q++;

        if ((q < end) && (*q == ':'))
        {
            foreach(LinuxPort* port, allPorts_)
            {
                if (strncmp(port->name(), p, int(q-p)) == 0)
                {
                    portStats[index] = &(port->stats_);

                    if (setPromisc(port->name()))
                        port->clearPromisc_ = true;
                    else
                        port->isPromisc_ = false;

                    break;
                }
            }
        }
        index++;

        // Skip till newline
        p = q;
        while (*p != '\n')
            p++;
        p++;
    }
    Q_ASSERT(index == count);

    qDebug("stats for %d ports setup", count);
    setupDone_ = true;

    //
    // We are all set - Let's start polling for stats!
    //
    while (!stop_)
    {
        lseek(fd, 0, SEEK_SET);
        len = read(fd, (void*) buf.data(), buf.size());
        if (len < 0)
        {
            if (buf.size() > 1*1024*1024)
            {
                qWarning("buffer size hit limit. no more stats");
                return;
            }
            qDebug("doubling buffer size. curr = %d", buf.size());
            buf.resize(buf.size() * 2);
            continue;
        }

        p = buf.data();
        end = p + len;

        // Skip first two lines
        while (*p != '\n')
            p++;
        p++;
        while (*p != '\n')
            p++;
        p++;

        index = 0;
        while (p < end)
        {
            uint dummy;
            quint64 rxBytes, rxPkts;
            quint64 rxErrors, rxDrops, rxFifo, rxFrame;
            quint64 txBytes, txPkts;

            // Skip interface name - we assume the number and order of ports
            // won't change since we parsed the output before we started polling
            while ((p < end) && (*p != ':') && (*p != '\n'))
                p++;
            if (p >= end)
                break;
            if (*p == '\n')
            {
                index++;
                continue;
            }
            p++;

            sscanf(p, fmt,
                    &rxBytes, &rxPkts, &rxErrors, &rxDrops, &rxFifo, &rxFrame, 
                        &dummy, &dummy,
                    &txBytes, &txPkts, &dummy, &dummy, &dummy, &dummy, &dummy, 
                        &dummy);

            if (index < count)
            {
                AbstractPort::PortStats *stats = portStats[index];
                if (stats)
                {
                    // TODO: fix the pps/Bps calc similar to netlink stats
                    stats->rxPps = 
                        ((rxPkts >= stats->rxPkts) ? 
                                rxPkts - stats->rxPkts : 
                                rxPkts + (kMaxValue32 - stats->rxPkts))
                        / kRefreshFreq_;
                    stats->rxBps = 
                        ((rxBytes >= stats->rxBytes) ? 
                                rxBytes - stats->rxBytes : 
                                rxBytes + (kMaxValue32 - stats->rxBytes))
                        / kRefreshFreq_;
                    stats->rxPkts  = rxPkts;
                    stats->rxBytes = rxBytes;
                    stats->txPps = 
                        ((txPkts >= stats->txPkts) ? 
                                txPkts - stats->txPkts : 
                                txPkts + (kMaxValue32 - stats->txPkts))
                        / kRefreshFreq_;
                    stats->txBps = 
                        ((txBytes >= stats->txBytes) ? 
                                txBytes - stats->txBytes : 
                                txBytes + (kMaxValue32 - stats->txBytes))
                        / kRefreshFreq_;
                    stats->txPkts  = txPkts;
                    stats->txBytes = txBytes;

                    stats->rxDrops = rxDrops;
                    stats->rxErrors = rxErrors;
                    stats->rxFifoErrors = rxFifo;
                    stats->rxFrameErrors = rxFrame;
                }
            }

            while (*p != '\n')
                p++;
            p++;
            index++;
        }
        QThread::sleep(kRefreshFreq_);
    }

    free(portStats);
}

int LinuxPort::StatsMonitor::netlinkStats()
{
    QHash<uint, PortStats*> portStats;
    QHash<uint, quint64*> portMaxStatsValue;
    QHash<uint, OstProto::LinkState*> linkState;
    int fd;
    struct sockaddr_nl local;
    struct sockaddr_nl kernel;
    QByteArray buf;
    int len, count;
    struct {
        struct nlmsghdr nlh;
        struct rtgenmsg rtg;
    } ifListReq;
    struct iovec iov;
    struct msghdr msg;
    struct nlmsghdr *nlm;
    bool done = false;

    //
    // We first setup stuff before we start polling for stats
    //
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0)
    {
        qWarning("Unable to open netlink socket (errno %d)", errno);
        return -1;
    }

    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;

    if (bind(fd, (struct sockaddr*) &local, sizeof(local)) < 0)
    {
        qWarning("Unable to bind netlink socket (errno %d)", errno);
        return -1;
    }

    memset(&ifListReq, 0, sizeof(ifListReq));
    ifListReq.nlh.nlmsg_len = sizeof(ifListReq);
    ifListReq.nlh.nlmsg_type = RTM_GETLINK;
    ifListReq.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    ifListReq.nlh.nlmsg_pid = 0;
    ifListReq.rtg.rtgen_family = AF_PACKET;

    buf.fill('\0', 1024);

    msg.msg_name = &kernel;
    msg.msg_namelen = sizeof(kernel);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    qDebug("nlmsg_flags = %x", ifListReq.nlh.nlmsg_flags);

    if (send(fd, (void*)&ifListReq, sizeof(ifListReq), 0) < 0)
    {
        qWarning("Unable to send GETLINK request (errno %d)", errno);
        return -1;
    }

    count = 0;

_retry:

    // Find required size of buffer and resize accordingly
    while (1)
    {
        iov.iov_base = buf.data();
        iov.iov_len = buf.size();
        msg.msg_flags = 0;

        // Peek at reply to check buffer size required
        len = recvmsg(fd, &msg, MSG_PEEK|MSG_TRUNC);

        if (len < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            qWarning("netlink recv error %d", errno);
            return -1;
        }
        else if (len == 0)
        {
            qWarning("netlink closed the socket on my face!");
            return -1;
        }
        else 
        {
            if (msg.msg_flags & MSG_TRUNC)
            {
                if (len == buf.size()) // Older Kernel returns truncated size
                {
                    qDebug("netlink buffer size %d not enough", buf.size());
                    qDebug("retrying with double the size");
                    // Double the size and retry
                    buf.resize(buf.size()*2);
                    continue;
                }
                else // Newer Kernel returns actual size required
                {
                    qDebug("netlink required buffer size = %d", len);
                    buf.resize(len);
                    continue;
                }
            }
            else
                qDebug("buffer size %d enough for netlink", buf.size());

            break;
        }
    }

    msg.msg_flags = 0;

    // Actually receive the reply now
    len = recvmsg(fd, &msg, 0);

    if (len < 0)
    {
        if (errno == EINTR || errno == EAGAIN)
            goto _retry;
        qWarning("netlink recv error %d", errno);
        return -1;
    }
    else if (len == 0)
    {
        qWarning("netlink socket closed unexpectedly");
        return -1;
    }

    //
    // Populate the port stats hash table
    //
    nlm = (struct nlmsghdr*) buf.data();
    while (NLMSG_OK(nlm, (uint)len))
    {
        struct ifinfomsg *ifi;
        struct rtattr *rta;
        int rtaLen;
        char ifname[64] = "";

        if (nlm->nlmsg_type == NLMSG_DONE)
        {
            done = true;
            break;
        }

        if (nlm->nlmsg_type == NLMSG_ERROR)
        {
            struct nlmsgerr *err = (struct nlmsgerr*) NLMSG_DATA(nlm);
            qDebug("RTNETLINK error %d", err->error);
            done = true;
            break;
        }

        Q_ASSERT(nlm->nlmsg_type == RTM_NEWLINK);

        ifi = (struct ifinfomsg*) NLMSG_DATA(nlm);
        rta = IFLA_RTA(ifi);
        rtaLen = len - NLMSG_LENGTH(sizeof(*ifi));
        while (RTA_OK(rta, rtaLen))
        {
            if (rta->rta_type == IFLA_IFNAME)
            {
                strncpy(ifname, (char*)RTA_DATA(rta), RTA_PAYLOAD(rta));
                ifname[RTA_PAYLOAD(rta)] = 0;
                break;
            }
            rta = RTA_NEXT(rta, rtaLen);
        }

        qDebug("if: %s(%d)", ifname, ifi->ifi_index);
        foreach(LinuxPort* port, allPorts_)
        {
            if (strcmp(port->name(), ifname) == 0)
            {
                portStats[uint(ifi->ifi_index)] = &(port->stats_);
                portMaxStatsValue[uint(ifi->ifi_index)] = 
                        &(port->maxStatsValue_);
                linkState[uint(ifi->ifi_index)] = &(port->linkState_);

                if (setPromisc(port->name()))
                    port->clearPromisc_ = true;
                else
                    port->isPromisc_ = false;

                count++;
                break;
            }
        }
        nlm = NLMSG_NEXT(nlm, len);
    }

    if (!done)
        goto _retry;

    qDebug("port count = %d\n", count);
    if (count <= 0)
    {
        qWarning("no ports in RTNETLINK GET_LINK - no stats will be available");
        return - 1;
    }

    qDebug("stats for %d ports setup", count);
    setupDone_ = true;

    //
    // We are all set - Let's start polling for stats!
    //
    while (!stop_)
    {
        if (send(fd, (void*)&ifListReq, sizeof(ifListReq), 0) < 0)
        {
            qWarning("Unable to send GETLINK request (errno %d)", errno);
            goto _try_later;
        }

        done = false;

_retry_recv:
        msg.msg_flags = 0;
        len = recvmsg(fd, &msg, 0);

        if (len < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                goto _retry_recv;
            qWarning("netlink recv error %d", errno);
            break;
        }
        else if (len == 0)
        {
            qWarning("netlink socket closed unexpectedly");
            break;
        }

        nlm = (struct nlmsghdr*) buf.data();
        while (NLMSG_OK(nlm, (uint)len))
        {
            struct ifinfomsg *ifi;
            struct rtattr *rta;
            int rtaLen;

            if (nlm->nlmsg_type == NLMSG_DONE) 
            {
                done = true;
                break;
            }

            if (nlm->nlmsg_type == NLMSG_ERROR)
            {
                struct nlmsgerr *err = (struct nlmsgerr*) NLMSG_DATA(nlm);
                qDebug("RTNETLINK error: %s", strerror(-err->error));
                done = true;
                break;
            }

            Q_ASSERT(nlm->nlmsg_type == RTM_NEWLINK);

            ifi = (struct ifinfomsg*) NLMSG_DATA(nlm);
            rta = IFLA_RTA(ifi);
            rtaLen = len - NLMSG_LENGTH(sizeof(*ifi));
            while (RTA_OK(rta, rtaLen))
            {
                if (rta->rta_type == X_IFLA_STATS)
                {
                    x_rtnl_link_stats *rtnlStats = 
                            (x_rtnl_link_stats*) RTA_DATA(rta);
                    AbstractPort::PortStats *stats = portStats[ifi->ifi_index];
                    quint64 *maxStatsValue = portMaxStatsValue[ifi->ifi_index];
                    OstProto::LinkState *state = linkState[ifi->ifi_index];

                    if (!stats)
                        break;

                    if (rtnlStats->rx_packets >= stats->rxPkts) {
                        stats->rxPps = (rtnlStats->rx_packets - stats->rxPkts)
                                            / kRefreshFreq_;
                    }
                    else {
                        if (*maxStatsValue == 0) {
                            *maxStatsValue = stats->rxPkts > kMaxValue32 ?
                                kMaxValue64 : kMaxValue32;
                        }
                        stats->rxPps = ((*maxStatsValue - stats->rxPkts)
                                            + rtnlStats->rx_packets)
                                        / kRefreshFreq_;
                    }

                    if (rtnlStats->rx_bytes >= stats->rxBytes) {
                        stats->rxBps = (rtnlStats->rx_bytes - stats->rxBytes)
                                            / kRefreshFreq_;
                    }
                    else {
                        if (*maxStatsValue == 0) {
                            *maxStatsValue = stats->rxBytes > kMaxValue32 ?
                                kMaxValue64 : kMaxValue32;
                        }
                        stats->rxBps = ((*maxStatsValue - stats->rxBytes)
                                            + rtnlStats->rx_bytes)
                                        / kRefreshFreq_;
                    }

                    stats->rxPkts  = rtnlStats->rx_packets;
                    stats->rxBytes = rtnlStats->rx_bytes;

                    if (rtnlStats->tx_packets >= stats->txPkts) {
                        stats->txPps = (rtnlStats->tx_packets - stats->txPkts)
                                            / kRefreshFreq_;
                    }
                    else {
                        if (*maxStatsValue == 0) {
                            *maxStatsValue = stats->txPkts > kMaxValue32 ?
                                kMaxValue64 : kMaxValue32;
                        }
                        stats->txPps = ((*maxStatsValue - stats->txPkts)
                                            + rtnlStats->tx_packets)
                                        / kRefreshFreq_;
                    }

                    if (rtnlStats->tx_bytes >= stats->txBytes) {
                        stats->txBps = (rtnlStats->tx_bytes - stats->txBytes)
                                            / kRefreshFreq_;
                    }
                    else {
                        if (*maxStatsValue == 0) {
                            *maxStatsValue = stats->txBytes > kMaxValue32 ?
                                kMaxValue64 : kMaxValue32;
                        }
                        stats->txBps = ((*maxStatsValue - stats->txBytes)
                                            + rtnlStats->tx_bytes)
                                        / kRefreshFreq_;
                    }

                    stats->txPkts  = rtnlStats->tx_packets;
                    stats->txBytes = rtnlStats->tx_bytes;

                    // TODO: export detailed error stats
                    stats->rxDrops =   rtnlStats->rx_dropped 
                                     + rtnlStats->rx_missed_errors;
                    stats->rxErrors = rtnlStats->rx_errors;
                    stats->rxFifoErrors = rtnlStats->rx_fifo_errors;
                    stats->rxFrameErrors =   rtnlStats->rx_crc_errors
                                           + rtnlStats->rx_length_errors
                                           + rtnlStats->rx_over_errors
                                           + rtnlStats->rx_frame_errors;

                    Q_ASSERT(state);  
                    *state = ifi->ifi_flags & IFF_RUNNING ?
                        OstProto::LinkStateUp : OstProto::LinkStateDown;

                    break;
                }
                rta = RTA_NEXT(rta, rtaLen);
            }
            nlm = NLMSG_NEXT(nlm, len);
        }

        if (!done)
            goto _retry_recv;

_try_later:
        QThread::sleep(kRefreshFreq_);
    }

    portStats.clear();
    linkState.clear();

    return 0;
}

int LinuxPort::StatsMonitor::setPromisc(const char * portName)
{ 
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, portName, sizeof(ifr.ifr_name));

    if (ioctl(ioctlSocket_, SIOCGIFFLAGS, &ifr) != -1)
    {
        if ((ifr.ifr_flags & IFF_PROMISC) == 0)
        {
            ifr.ifr_flags |= IFF_PROMISC;
            if (ioctl(ioctlSocket_, SIOCSIFFLAGS, &ifr) != -1)
            {
                return 1;
            }
            else
            {
                qDebug("%s: failed to set promisc; "
                        "SIOCSIFFLAGS failed (%s)", 
                        portName, strerror(errno));
            }
        }
    }
    else
    {
        qDebug("%s: failed to set promisc; SIOCGIFFLAGS failed (%s)",
                portName, strerror(errno));
    }

    return 0;
}

void LinuxPort::StatsMonitor::stop()
{
    stop_ = true;
}

bool LinuxPort::StatsMonitor::waitForSetupFinished(int msecs)
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
