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

#include "bsdhostdevice.h"
#include "netdefs.h"
#include "packetbuffer.h"

#include <QHostAddress>

#ifdef Q_OS_BSD4

#include "../common/qtport.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <pcap.h>
#include <unistd.h>

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

quint32 sumUInt128(UInt128 value);

BsdHostDevice::BsdHostDevice(QString portName,
        DeviceManager *deviceManager)
    : Device(deviceManager)
{
    ifName_ = portName;
    ifIndex_ = if_nametoindex(qPrintable(ifName_));
    qDebug("Port %s: ifIndex %d", qPrintable(ifName_), ifIndex_);

    rtSock_ = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);
    shutdown(rtSock_, SHUT_RD); // we don't read from rtSock

    char errbuf[PCAP_ERRBUF_SIZE] = "";
    txHandle_ = pcap_open_live(qPrintable(ifName_), 64, 0, 0, errbuf);
    if (txHandle_ == NULL) {
        qWarning("pcap open %s failed (%s)", qPrintable(ifName_), errbuf);
    }
}

void BsdHostDevice::receivePacket(PacketBuffer* /*pktBuf*/)
{
    // Do Nothing
}

void BsdHostDevice::clearNeighbors(Device::NeighborSet set)
{
    // No need to do anything - see AbstractPort::resolveDeviceNeighbors()
    // on when this is used
    if (set == kUnresolvedNeighbors)
        return;

    size_t len;
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, 0};
    const int mibLen = sizeof(mib)/sizeof(mib[0]);
    QByteArray buf;

#if defined(RTF_LLDATA)
    mib[5] = RTF_LLDATA;
#else
    mib[5] = RTF_LLINFO;
#endif

    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(1) failed (%s)\n", strerror(errno));
        return;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(2) failed(%s)\n", strerror(errno));
        return;
    }

    int count=0, fail=0;
    char *p = buf.data();
    const char *end = p + len;
    while (p < end)
    {
        struct rt_msghdr *rtm = (struct rt_msghdr*) p;

        if ((rtm->rtm_index == ifIndex_) && !(rtm->rtm_flags & RTF_PINNED)) {
            const struct sockaddr *sa = (const struct sockaddr*)(rtm + 1);
            rtm->rtm_type = RTM_DELETE;
            if (write(rtSock_, p, rtm->rtm_msglen) < 0) {
                qWarning("RTM_DELETE failed for ip %s (%s)",
                        qPrintable(QHostAddress(sa).toString()) , strerror(errno));
                fail++;
            }
            count++;
        }
        p += rtm->rtm_msglen;
    }
    qDebug("Flush ARP table for ifIndex %u: %d/%d deleted",
             ifIndex_, count - fail, count);

    // We need to query AF_INET and AF_INET6 separately as sysctl with AF_UNSPEC
    // doesn't work
    mib[3] = AF_INET6;
    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(3) failed (%s)\n", strerror(errno));
        return;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(4) failed(%s)\n", strerror(errno));
        return;
    }

    count = fail = 0;
    p = buf.data();
    end = p + len;
    while (p < end)
    {
        struct rt_msghdr *rtm = (struct rt_msghdr*) p;

        if ((rtm->rtm_index == ifIndex_) && !(rtm->rtm_flags & RTF_PINNED)) {
            const struct sockaddr *sa = (const struct sockaddr*)(rtm + 1);
            rtm->rtm_type = RTM_DELETE;
            if (write(rtSock_, p, rtm->rtm_msglen) < 0) {
                qWarning("RTM_DELETE failed for ip %s (%s)",
                        qPrintable(QHostAddress(sa).toString()) , strerror(errno));
                fail++;
            }
            count++;
        }
        p += rtm->rtm_msglen;
    }
    qDebug("Flush ND table for ifIndex %u: %d/%d deleted",
             ifIndex_, count - fail, count);
}

void BsdHostDevice::getNeighbors(OstEmul::DeviceNeighborList *neighbors)
{
    size_t len;
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, 0};
    const int mibLen = sizeof(mib)/sizeof(mib[0]);
    QByteArray buf;

#if defined(RTF_LLDATA)
    mib[5] = RTF_LLDATA;
#else
    mib[5] = RTF_LLINFO;
#endif

    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(1) failed (%s)\n", strerror(errno));
        return;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(2) failed(%s)\n", strerror(errno));
        return;
    }

    const char *p = buf.constData();
    const char *end = p + len;
    while (p < end)
    {
        const struct rt_msghdr *rtm = (const struct rt_msghdr*) p;
        const struct sockaddr_in *sin = (const struct sockaddr_in*)(rtm + 1);
        const struct sockaddr_dl *sdl = (const struct sockaddr_dl*)
                                            ((char*)sin + SA_SIZE(sin));
        if (sdl->sdl_index == ifIndex_) {
            OstEmul::ArpEntry *arp = neighbors->add_arp();
            arp->set_ip4(qFromBigEndian<quint32>(sin->sin_addr.s_addr));
            arp->set_mac(qFromBigEndian<quint64>(LLADDR(sdl)) >> 16);
        }
        p += rtm->rtm_msglen;
    }

    // We need to query AF_INET and AF_INET6 separately as sysctl with AF_UNSPEC
    // doesn't work
    mib[3] = AF_INET6;
    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(1) failed (%s)\n", strerror(errno));
        return;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(2) failed(%s)\n", strerror(errno));
        return;
    }

    p = buf.constData();
    end = p + len;
    while (p < end)
    {
        const struct rt_msghdr *rtm = (const struct rt_msghdr*) p;
        const struct sockaddr_in6 *sin = (const struct sockaddr_in6*)(rtm + 1);
        const struct sockaddr_dl *sdl = (const struct sockaddr_dl*)
                                            ((char*)sin + SA_SIZE(sin));
        if (sdl->sdl_index == ifIndex_) {
#ifdef __KAME__
            if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr)
                    || IN6_IS_ADDR_MC_LINKLOCAL(&sin->sin6_addr)) {
                // remove the embedded ifIndex in the 2nd hextet (u16)
                const_cast<struct sockaddr_in6*>(sin)->sin6_addr.s6_addr[2] = 0;
                const_cast<struct sockaddr_in6*>(sin)->sin6_addr.s6_addr[3] = 0;
#endif
            }
            OstEmul::NdpEntry *ndp = neighbors->add_ndp();
            ndp->mutable_ip6()->set_hi(qFromBigEndian<quint64>(sin->sin6_addr.s6_addr));
            ndp->mutable_ip6()->set_lo(qFromBigEndian<quint64>(sin->sin6_addr.s6_addr+8));
            ndp->set_mac(qFromBigEndian<quint64>(LLADDR(sdl)) >> 16);
        }
        p += rtm->rtm_msglen;
    }
}

quint64 BsdHostDevice::arpLookup(quint32 ip)
{
    quint64 mac = 0;
    size_t len;
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, 0};
    const int mibLen = sizeof(mib)/sizeof(mib[0]);
    QByteArray buf;

#if defined(RTF_LLDATA)
    mib[5] = RTF_LLDATA;
#else
    mib[5] = RTF_LLINFO;
#endif

    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(1) failed (%s)\n", strerror(errno));
        return mac;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(2) failed(%s)\n", strerror(errno));
        return mac;
    }

    const char *p = buf.constData();
    const char *end = p + len;
    while (p < end)
    {
        const struct rt_msghdr *rtm = (const struct rt_msghdr*) p;

        if (rtm->rtm_index == ifIndex_) {
            const struct sockaddr_in *sin = (const struct sockaddr_in*)(rtm + 1);
            if (qFromBigEndian<quint32>(sin->sin_addr.s_addr) == ip) {
                const struct sockaddr_dl *sdl = (const struct sockaddr_dl*)
                                                    ((char*)sin + SA_SIZE(sin));
                mac = qFromBigEndian<quint64>(LLADDR(sdl)) >> 16;
                break;
            }
        }
        p += rtm->rtm_msglen;
    }
    return mac;
}

quint64 BsdHostDevice::ndpLookup(UInt128 ip)
{
    quint64 mac = 0;
    size_t len;
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET6, NET_RT_FLAGS, 0};
    const int mibLen = sizeof(mib)/sizeof(mib[0]);
    QByteArray buf;

#if defined(RTF_LLDATA)
    mib[5] = RTF_LLDATA;
#else
    mib[5] = RTF_LLINFO;
#endif

    if (sysctl(mib, mibLen, NULL, &len, NULL, 0) < 0) {  // find buffer len
        qWarning("sysctl NET_RT_FLAGS(1) failed (%s)\n", strerror(errno));
        return mac;
    }

    buf.resize(len);
    if (sysctl(mib, mibLen, buf.data(), &len, NULL, 0) < 0) { // now retreive ARP/NDP
        qWarning("sysctl NET_RT_FLAGS(2) failed(%s)\n", strerror(errno));
        return mac;
    }

    const char *p = buf.constData();
    const char *end = p + len;
    while (p < end)
    {
        const struct rt_msghdr *rtm = (const struct rt_msghdr*) p;

        if (rtm->rtm_index == ifIndex_) {
            const struct sockaddr_in6 *sin = (const struct sockaddr_in6*)(rtm + 1);
            if ((qFromBigEndian<quint64>(sin->sin6_addr.s6_addr) == ip.hi64())
                && (qFromBigEndian<quint64>(sin->sin6_addr.s6_addr+8) == ip.lo64())) {
                const struct sockaddr_dl *sdl = (const struct sockaddr_dl*)
                                                    ((char*)sin + SA_SIZE(sin));
                mac = qFromBigEndian<quint64>(LLADDR(sdl)) >> 16;
                break;
            }
        }
        p += rtm->rtm_msglen;
    }

    return mac;
}

void BsdHostDevice::sendArpRequest(quint32 tgtIp)
{
    //
    // XXX: I can't seem to find a BSD syscall to trigger the kernel to send an ARP;
    // so for now craft one from scratch and send
    // NOTE: RTM_RESOLVE has been removed - see
    // http://conferences.sigcomm.org/sigcomm/2009/workshops/presto/papers/p37.pdf
    //
    quint32 srcIp = ip4_;
    PacketBuffer *reqPkt;
    uchar *pktData;

    // Validate target IP
    if (!tgtIp)
        return;

    reqPkt = new PacketBuffer;
    reqPkt->reserve(encapSize());
    pktData = reqPkt->put(28);
    if (pktData) {
        // HTYP, PTYP
        *(quint32*)(pktData   ) = qToBigEndian(quint32(0x00010800));
        // HLEN, PLEN, OPER
        *(quint32*)(pktData+ 4) = qToBigEndian(quint32(0x06040001));
        // Source H/W Addr, Proto Addr
        *(quint32*)(pktData+ 8) = qToBigEndian(quint32(mac_ >> 16));
        *(quint16*)(pktData+12) = qToBigEndian(quint16(mac_ & 0xffff));
        *(quint32*)(pktData+14) = qToBigEndian(srcIp);
        // Target H/W Addr, Proto Addr
        *(quint32*)(pktData+18) = qToBigEndian(quint32(0));
        *(quint16*)(pktData+22) = qToBigEndian(quint16(0));
        *(quint32*)(pktData+24) = qToBigEndian(tgtIp);
    }

    ethEncap(reqPkt, kBcastMac, kEthTypeArp);
    pcap_sendpacket((pcap_t*)txHandle_, reqPkt->data(), reqPkt->length());

    qDebug("Sent ARP Request for srcIp/tgtIp=%s/%s",
            qPrintable(QHostAddress(srcIp).toString()),
            qPrintable(QHostAddress(tgtIp).toString()));
}

void BsdHostDevice::sendNeighborSolicit(UInt128 tgtIp)
{
    // XXX: See note in sendArpRequest() - applies here too

    UInt128 dstIp, srcIp = ip6_;
    PacketBuffer *reqPkt;
    uchar *pktData;

    // Validate target IP
    if (tgtIp == UInt128(0, 0))
        return;

    // Form the solicited node address to be used as dstIp
    // ff02::1:ffXX:XXXX/104
    dstIp = UInt128((quint64(0xff02) << 48),
                    (quint64(0x01ff) << 24) | (tgtIp.lo64() & 0xFFFFFF));

    reqPkt = new PacketBuffer;
    reqPkt->reserve(encapSize() + kIp6HdrLen);
    pktData = reqPkt->put(32);
    if (pktData) {
        // Calculate checksum first -
        // start with fixed fields in ICMP Header and IPv6 Pseudo Header ...
        quint32 sum = 0x8700 + 0x0101 + 32 + kIpProtoIcmp6;

        // then variable fields from ICMP header ...
        sum += sumUInt128(tgtIp);
        sum += (mac_ >> 32) + ((mac_ >> 16) & 0xffff) + (mac_ & 0xffff);

        // and variable fields from IPv6 pseudo header
        sum += sumUInt128(ip6_);
        sum += sumUInt128(dstIp);

        while(sum >> 16)
            sum = (sum & 0xFFFF) + (sum >> 16);

        // Type, Code
        *(quint16*)(pktData   ) = qToBigEndian(quint16(0x8700));
        // Checksum
        *(quint16*)(pktData+ 2) = qToBigEndian(quint16(~sum));
        // Reserved
        *(quint32*)(pktData+ 4) = qToBigEndian(quint32(0));
        // Target IP
        memcpy(pktData+ 8, tgtIp.toArray(), 16);
        // Source Addr TLV + MacAddr
        *(quint16*)(pktData+24) = qToBigEndian(quint16(0x0101));
        *(quint32*)(pktData+26) = qToBigEndian(quint32(mac_ >> 16));
        *(quint16*)(pktData+30) = qToBigEndian(quint16(mac_ & 0xffff));
    }
    int payloadLen = reqPkt->length();
    uchar *p = reqPkt->push(kIp6HdrLen);
    quint64 dstMac = (quint64(0x3333) << 32) | (dstIp.lo64() & 0xffffffff);

    // Ver(4), TrfClass(8), FlowLabel(8)
    *(quint32*)(p   ) = qToBigEndian(quint32(0x60000000));
    *(quint16*)(p+ 4) = qToBigEndian(quint16(payloadLen));
    p[6] = kIpProtoIcmp6;  // protocol
    p[7] = 255; // HopLimit
    memcpy(p+ 8,  ip6_.toArray(), 16); // Source IP
    memcpy(p+24, dstIp.toArray(), 16); // Destination IP

    ethEncap(reqPkt, dstMac, kEthTypeIp6);
    pcap_sendpacket((pcap_t*)txHandle_, reqPkt->data(), reqPkt->length());

    qDebug("Sent NDP Request for srcIp/tgtIp=%s/%s",
            qPrintable(QHostAddress(srcIp.toArray()).toString()),
            qPrintable(QHostAddress(tgtIp.toArray()).toString()));
}

int BsdHostDevice::encapSize()
{
    Q_ASSERT(numVlanTags_ >= 0);
    // ethernet header + vlans
    int size = 14 + kMaxVlan*numVlanTags_;

    return size;
}

void BsdHostDevice::ethEncap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type)
{
    int ofs;
    quint64 srcMac = mac_;
    uchar *p = pktBuf->push(encapSize());

    if (!p) {
        qWarning("%s: failed to push %d bytes [0x%p, 0x%p]", __FUNCTION__,
                encapSize(), pktBuf->head(), pktBuf->data());
        goto _exit;
    }

    *(quint32*)(p     ) =  qToBigEndian(quint32(dstMac >> 16));
    *(quint16*)(p +  4) =  qToBigEndian(quint16(dstMac & 0xffff));
    *(quint32*)(p +  6) =  qToBigEndian(quint32(srcMac >> 16));
    *(quint16*)(p + 10) =  qToBigEndian(quint16(srcMac & 0xffff));
    ofs = 12;
    for (int i = 0; i < numVlanTags_; i++) {
        *(quint32*)(p + ofs) =  qToBigEndian(vlan_[i]);
        ofs += 4;
    }
    *(quint16*)(p + ofs) =  qToBigEndian(type);
    ofs += 2;

    Q_ASSERT(ofs == encapSize());

_exit:
    return;
}
#endif
