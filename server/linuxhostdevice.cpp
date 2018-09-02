/*
Copyright (C) 2018 Srivats P.

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

#include "linuxhostdevice.h"

#ifdef Q_OS_LINUX

#include "../common/qtport.h"

#include <QHostAddress>

#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>

LinuxHostDevice::LinuxHostDevice(QString portName,
        DeviceManager *deviceManager)
    : Device(deviceManager)
{
    ifName_ = portName;

    netSock_ = nl_socket_alloc();
    if (!netSock_) {
        qWarning("Failed to open netlink socket for %s", qPrintable(ifName_));
        return;
    }

    if (nl_connect(netSock_, NETLINK_ROUTE) < 0) {
        qWarning("Failed to connect netlink socket for %s", qPrintable(ifName_));
        return;
    }

    rtnl_link *link;
    if (rtnl_link_get_kernel(netSock_, 0, qPrintable(ifName_), &link) < 0) {
        qWarning("Failed to get rtnet link from kernel for %s", qPrintable(ifName_));
        return;
    }

    ifIndex_ = rtnl_link_get_ifindex(link);
    qDebug("Port %s: ifIndex %d", qPrintable(ifName_), ifIndex_);

    rtnl_link_put(link);
}

void LinuxHostDevice::receivePacket(PacketBuffer* /*pktBuf*/)
{
    // Do Nothing
}

void LinuxHostDevice::clearNeighbors(Device::NeighborSet set)
{
    // No need to do anything - see AbstractPort::resolveDeviceNeighbors()
    // on when this is used
    if (set == kUnresolvedNeighbors)
        return;

    nl_cache *neighCache;
    if (rtnl_neigh_alloc_cache(netSock_, &neighCache) < 0) {
        qWarning("Failed to get neigh cache from kernel");
        return;
    }

    if (!neighCache) {
        qWarning("Neigh cache empty");
        return;
    }

    int count=0, fail=0;
    rtnl_neigh *neigh = (rtnl_neigh*) nl_cache_get_first(neighCache);
    while (neigh) {
        if ((rtnl_neigh_get_ifindex(neigh) == ifIndex_) 
                && (rtnl_neigh_get_family(neigh) == AF_INET
                    || rtnl_neigh_get_family(neigh) == AF_INET6)
                && !(rtnl_neigh_get_state(neigh) & (NUD_PERMANENT|NUD_NOARP))) {
            count++;
            if (rtnl_neigh_delete(netSock_, neigh, 0) < 0)
                fail++;
        }
        neigh = (rtnl_neigh*) nl_cache_get_next(OBJ_CAST(neigh));
    }
    nl_cache_put(neighCache);
    qDebug("Flush ARP/ND table for ifIndex %u: %d/%d deleted",
             ifIndex_, count - fail, count);
}

void LinuxHostDevice::getNeighbors(OstEmul::DeviceNeighborList *neighbors)
{
    nl_cache *neighCache;
    if (rtnl_neigh_alloc_cache(netSock_, &neighCache) < 0) {
        qWarning("Failed to get neigh cache from kernel");
        return;
    }

    if (!neighCache) {
        qWarning("Neigh cache empty");
        return;
    }

    rtnl_neigh *neigh = (rtnl_neigh*) nl_cache_get_first(neighCache);
    while (neigh) {
        if ((rtnl_neigh_get_ifindex(neigh) == ifIndex_)
            && !(rtnl_neigh_get_state(neigh) & NUD_NOARP)) {
            if (rtnl_neigh_get_family(neigh) == AF_INET) {
                OstEmul::ArpEntry *arp = neighbors->add_arp();
                arp->set_ip4(qFromBigEndian<quint32>(
                            nl_addr_get_binary_addr(rtnl_neigh_get_dst(neigh))));
                nl_addr *lladdr = rtnl_neigh_get_lladdr(neigh);
                arp->set_mac(lladdr ? 
                                qFromBigEndian<quint64>(
                                    nl_addr_get_binary_addr(lladdr)) >> 16 :
                                0);
            }
            else if (rtnl_neigh_get_family(neigh) == AF_INET6) {
                OstEmul::NdpEntry *ndp = neighbors->add_ndp();
                ndp->mutable_ip6()->set_hi(qFromBigEndian<quint64>(
                                nl_addr_get_binary_addr(
                                                    rtnl_neigh_get_dst(neigh))));
                ndp->mutable_ip6()->set_lo(qFromBigEndian<quint64>((const uchar*)
                                nl_addr_get_binary_addr(
                                                    rtnl_neigh_get_dst(neigh))+8));
                nl_addr *lladdr = rtnl_neigh_get_lladdr(neigh);
                ndp->set_mac(lladdr ? 
                                qFromBigEndian<quint64>(
                                    nl_addr_get_binary_addr(lladdr)) >> 16 :
                                0);
            }
        }
        neigh = (rtnl_neigh*) nl_cache_get_next(OBJ_CAST(neigh));
    }
    nl_cache_put(neighCache);
}

quint64 LinuxHostDevice::arpLookup(quint32 ip)
{
    quint64 mac = 0;
    nl_cache *neighCache;
    if (rtnl_neigh_alloc_cache(netSock_, &neighCache) < 0) {
        qWarning("Failed to get neigh cache from kernel");
        return mac;
    }

    if (!neighCache) {
        qWarning("Neigh cache empty");
        return mac;
    }

    quint32 ipBig = qToBigEndian(ip);
    nl_addr *dst = nl_addr_build(AF_INET, &ipBig, sizeof(ipBig));
#if 0
    //
    // libnl 3.2.[15..21] have a bug in rtnl_neigh_get and fail to find entry
    // https://github.com/tgraf/libnl/commit/8571f58f23763d8db7365d02c9b27832ad3d7005
    //
    rtnl_neigh *neigh = rtnl_neigh_get(neighCache, ifIndex_, dst);
    if (neigh) {
        mac = qFromBigEndian<quint64>(
                nl_addr_get_binary_addr(rtnl_neigh_get_lladdr(neigh))) >> 16;
        rtnl_neigh_put(neigh);
    }
#else
    rtnl_neigh *neigh = (rtnl_neigh*) nl_cache_get_first(neighCache);
    while (neigh) {
        if ((rtnl_neigh_get_ifindex(neigh) == ifIndex_)
                && (rtnl_neigh_get_family(neigh) == AF_INET)
                && !nl_addr_cmp(rtnl_neigh_get_dst(neigh), dst)) {
            nl_addr *lladdr = rtnl_neigh_get_lladdr(neigh);
            if (lladdr)
                mac = qFromBigEndian<quint64>(
                        nl_addr_get_binary_addr(lladdr)) >> 16;
            break;
        }
        neigh = (rtnl_neigh*) nl_cache_get_next(OBJ_CAST(neigh));
    }
#endif
    nl_addr_put(dst);
    nl_cache_put(neighCache);

    return mac;
}

quint64 LinuxHostDevice::ndpLookup(UInt128 ip)
{
    quint64 mac = 0;
    nl_cache *neighCache;
    if (rtnl_neigh_alloc_cache(netSock_, &neighCache) < 0) {
        qWarning("Failed to get neigh cache from kernel");
        return mac;
    }

    if (!neighCache) {
        qWarning("Neigh cache empty");
        return mac;
    }

    nl_addr *dst = nl_addr_build(AF_INET6, ip.toArray(), 16);
#if 0
    //
    // libnl 3.2.[15..21] have a bug in rtnl_neigh_get and fail to find entry
    // https://github.com/tgraf/libnl/commit/8571f58f23763d8db7365d02c9b27832ad3d7005
    //
    rtnl_neigh *neigh = rtnl_neigh_get(neighCache, ifIndex_, dst);
    if (neigh) {
        mac = qFromBigEndian<quint64>(
                nl_addr_get_binary_addr(rtnl_neigh_get_lladdr(neigh))) >> 16;
        rtnl_neigh_put(neigh);
    }
#else
    rtnl_neigh *neigh = (rtnl_neigh*) nl_cache_get_first(neighCache);
    while (neigh) {
        if ((rtnl_neigh_get_ifindex(neigh) == ifIndex_)
                && (rtnl_neigh_get_family(neigh) == AF_INET6)
                && !nl_addr_cmp(rtnl_neigh_get_dst(neigh), dst)) {
            nl_addr *lladdr = rtnl_neigh_get_lladdr(neigh);
            if (lladdr)
                mac = qFromBigEndian<quint64>(
                        nl_addr_get_binary_addr(lladdr)) >> 16;
            break;
        }
        neigh = (rtnl_neigh*) nl_cache_get_next(OBJ_CAST(neigh));
    }
#endif
    nl_addr_put(dst);
    nl_cache_put(neighCache);

    return mac;
}

void LinuxHostDevice::sendArpRequest(quint32 tgtIp)
{
    quint32 ipBig = qToBigEndian(tgtIp);
    nl_addr *dst = nl_addr_build(AF_INET, &ipBig, sizeof(ipBig));
    rtnl_neigh *neigh = rtnl_neigh_alloc();
    rtnl_neigh_set_ifindex(neigh, ifIndex_);
    rtnl_neigh_set_state(neigh, NUD_NONE);
    rtnl_neigh_set_dst(neigh, dst);
    rtnl_neigh_set_flags(neigh, NTF_USE); // force kernel to send ARP request
    if (int err = rtnl_neigh_add(netSock_, neigh, NLM_F_CREATE) < 0)
        qWarning("Resolve arp failed for port %s ip %08x: %s",
                qPrintable(ifName_), tgtIp, strerror(err));
    rtnl_neigh_put(neigh);
    nl_addr_put(dst);
}

void LinuxHostDevice::sendNeighborSolicit(UInt128 tgtIp)
{
    nl_addr *dst = nl_addr_build(AF_INET6, tgtIp.toArray(), 16);
    rtnl_neigh *neigh = rtnl_neigh_alloc();
    rtnl_neigh_set_ifindex(neigh, ifIndex_);
    rtnl_neigh_set_state(neigh, NUD_NONE);
    rtnl_neigh_set_dst(neigh, dst);
    rtnl_neigh_set_flags(neigh, NTF_USE); // force kernel to send ARP request
    if (int err = rtnl_neigh_add(netSock_, neigh, NLM_F_CREATE) < 0)
        qWarning("Resolve ndp failed for port %s ip %016llx-%016llx: %s",
                qPrintable(ifName_), tgtIp.hi64(), tgtIp.lo64(), strerror(err));
    rtnl_neigh_put(neigh);
    nl_addr_put(dst);
}

#endif
