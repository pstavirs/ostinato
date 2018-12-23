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

#include "winhostdevice.h"

#include <QUuid>

#ifdef Q_OS_WIN32

static WCHAR errBuf[256];
static inline QString errStr(ulong err)
{
    return FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  errBuf, sizeof(errBuf)-1, NULL) > 0 ?
        QString("error 0x%1 %2").arg(err, 0, 16)
                          .arg(QString().fromWCharArray(errBuf)) :
        QString("error 0x%1").arg(err, 0, 16);
}

WindowsHostDevice::WindowsHostDevice(QString portName,
        DeviceManager *deviceManager)
    : Device(deviceManager)
{
    GUID guid = static_cast<GUID>(QUuid(portName.right(38)));
    ulong status = ConvertInterfaceGuidToLuid(&guid, &luid_);
    if (status != NO_ERROR) {
        qWarning("ConvertInterfaceGuidToLuid failed for %s: %s",
                 qPrintable(portName), qPrintable(errStr(status)));
        luid_.Value = 0;
    }
    qInfo("Port %s: Luid %llx", qPrintable(portName), luid_.Value);
}

void WindowsHostDevice::receivePacket(PacketBuffer* /*pktBuf*/)
{
    // Do Nothing
}

void WindowsHostDevice::clearNeighbors(Device::NeighborSet set)
{
    // No need to do anything - see AbstractPort::resolveDeviceNeighbors()
    // on when this is used
    if (set == kUnresolvedNeighbors)
        return;

    NET_IFINDEX ifIndex;
    ulong status = ConvertInterfaceLuidToIndex(&luid_, &ifIndex);
    if (status != NO_ERROR) {
        qWarning("luid2ifIdx convert failed for LUID %llx: %s",
                 luid_.Value, qPrintable(errStr(status)));
        return;
    }

    status = FlushIpNetTable2(AF_UNSPEC, ifIndex);
    if(status != NO_ERROR)
        qWarning("Flush ARP/ND table failed for LUID %llx: %s",
                 luid_.Value, qPrintable(errStr(status)));
}

void WindowsHostDevice::getNeighbors(OstEmul::DeviceNeighborList *neighbors)
{
    PMIB_IPNET_TABLE2 nbrs = NULL;
    ADDRESS_FAMILY af = AF_UNSPEC;

    // TODO: the following can potentially be used elsewhere
    // but definition of AF_XXX may be different in different
    // platforms
    if (!hasIp4_)
        if (!hasIp6_)
            return;
        else
            af = AF_INET6;
    else if (!hasIp6_)
        af = AF_INET;

    ulong status = GetIpNetTable2(af, &nbrs) != NO_ERROR;
    if (status != NO_ERROR) {
        qWarning("Get ARP/ND table failed for LUID %llx: %s",
                 luid_.Value, qPrintable(errStr(status)));
        return;
    }

    for (uint i = 0; i < nbrs->NumEntries; i++) {
        if (nbrs->Table[i].InterfaceLuid.Value != luid_.Value)
            continue;

        if (nbrs->Table[i].Address.si_family == AF_INET) {
            OstEmul::ArpEntry *arp = neighbors->add_arp();
            arp->set_ip4(qFromBigEndian<quint32>(
                            nbrs->Table[i].Address.Ipv4.sin_addr.s_addr));
            arp->set_mac(qFromBigEndian<quint64>(
                            nbrs->Table[i].PhysicalAddress) >> 16);
        }
        else if (nbrs->Table[i].Address.si_family == AF_INET6) {
            OstEmul::NdpEntry *ndp = neighbors->add_ndp();
            ndp->mutable_ip6()->set_hi(qFromBigEndian<quint64>(
                            nbrs->Table[i].Address.Ipv6.sin6_addr.u.Byte));
            ndp->mutable_ip6()->set_lo(qFromBigEndian<quint64>(
                            nbrs->Table[i].Address.Ipv6.sin6_addr.u.Byte+8));
            ndp->set_mac(qFromBigEndian<quint64>(
                            nbrs->Table[i].PhysicalAddress) >> 16);
        }
    }

    FreeMibTable(nbrs);
}

quint64 WindowsHostDevice::arpLookup(quint32 ip)
{
    if (!luid_.Value)
        return 0;

    MIB_IPNET_ROW2 arpEntry;
    arpEntry.InterfaceLuid = luid_;
    arpEntry.Address.si_family = AF_INET;
    arpEntry.Address.Ipv4.sin_addr.s_addr = qToBigEndian(ip);

    if ((GetIpNetEntry2(&arpEntry) == NO_ERROR)
            && (arpEntry.PhysicalAddressLength == 6)) {
        return qFromBigEndian<quint64>(arpEntry.PhysicalAddress) >> 16;
    }
    else
        return 0;
}

quint64 WindowsHostDevice::ndpLookup(UInt128 ip)
{
    if (!luid_.Value)
        return 0;

    MIB_IPNET_ROW2 ndpEntry;
    ndpEntry.InterfaceLuid = luid_;
    ndpEntry.Address.si_family = AF_INET6;
    memcpy(&ndpEntry.Address.Ipv6.sin6_addr.u, ip.toArray(), 16);

    if ((GetIpNetEntry2(&ndpEntry) == NO_ERROR)
            && (ndpEntry.PhysicalAddressLength == 6)) {
        return qFromBigEndian<quint64>(ndpEntry.PhysicalAddress) >> 16;
    }
    else
        return 0;
}

void WindowsHostDevice::sendArpRequest(quint32 tgtIp)
{
    SOCKADDR_INET src;
    src.Ipv4.sin_addr.s_addr = qToBigEndian(ip4_);

    MIB_IPNET_ROW2 arpEntry;
    arpEntry.InterfaceLuid = luid_;
    arpEntry.Address.si_family = AF_INET;
    arpEntry.Address.Ipv4.sin_addr.s_addr = qToBigEndian(tgtIp);

    ulong status = ResolveIpNetEntry2(&arpEntry, &src);
    if (ResolveIpNetEntry2(&arpEntry, &src) != NO_ERROR)
        qWarning("Resolve arp failed for LUID %llx: %s",
                 luid_.Value, qPrintable(errStr(status)));
}

void WindowsHostDevice::sendNeighborSolicit(UInt128 tgtIp)
{
    SOCKADDR_INET src;
    memcpy(&src.Ipv6.sin6_addr.u, ip6_.toArray(), 16);

    MIB_IPNET_ROW2 ndpEntry;
    ndpEntry.InterfaceLuid = luid_;
    ndpEntry.Address.si_family = AF_INET6;
    memcpy(&ndpEntry.Address.Ipv6.sin6_addr.u, tgtIp.toArray(), 16);

    ulong status = ResolveIpNetEntry2(&ndpEntry, &src);
    if (ResolveIpNetEntry2(&ndpEntry, &src) != NO_ERROR)
        qWarning("Resolve ndp failed for LUID %llx: %s",
                 luid_.Value, qPrintable(errStr(status)));
}

#endif
