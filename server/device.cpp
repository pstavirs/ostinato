/*
Copyright (C) 2015 Srivats P.

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

#include "device.h"

#include "../common/emulproto.pb.h"
#include "devicemanager.h"
#include "packetbuffer.h"

#include <QHostAddress>
#include <qendian.h>

const int kBaseHex = 16;
const quint64 kBcastMac = 0xffffffffffffULL;
const quint16 kEthTypeArp = 0x0806;
const quint16 kEthTypeIp4 = 0x0800;
const quint16 kEthTypeIp6 = 0x86dd;
const int kIp6HdrLen = 40;
const quint8 kIpProtoIcmp6 = 58;

/*
 * NOTE:
 * 1. Device Key is (VLANS + MAC) - is assumed to be unique for a device
 * 2. Device clients/users (viz. DeviceManager) should take care when
 *    setting params that change the key, if the key is used elsewhere
 *    (e.g. in a hash)
 */

inline quint32 sumUInt128(UInt128 value)
{
    quint8 *arr = value.toArray();
    quint32 sum = 0;

    for (int i = 0; i < 16; i += 2)
        sum += qToBigEndian(*((quint16*)(arr + i)));

    return sum;
}

inline bool isIp6Mcast(UInt128 ip)
{
    return (ip.hi64() >> 56) == 0xff;
}

Device::Device(DeviceManager *deviceManager)
{
    deviceManager_ = deviceManager;

    for (int i = 0; i < kMaxVlan; i++)
        vlan_[i] = 0;
    numVlanTags_ = 0;
    mac_ = 0;

    hasIp4_ = false;
    hasIp6_ = false;

    clearKey();
}

void Device::setVlan(int index, quint16 vlan, quint16 tpid)
{
    int ofs;

    if ((index < 0) || (index >= kMaxVlan)) {
        qWarning("%s: vlan index %d out of range (0 - %d)", __FUNCTION__,
                index, kMaxVlan - 1);
        return;
    }

    vlan_[index] = (tpid << 16) | vlan;

    ofs = index * sizeof(quint16);
    key_[ofs]   = vlan >> 8;
    key_[ofs+1] = vlan & 0xff;

    if (index >= numVlanTags_)
        numVlanTags_ = index + 1;
}

quint64 Device::mac()
{
    return mac_;
}

void Device::setMac(quint64 mac)
{
    int ofs = kMaxVlan * sizeof(quint16);

    mac_ = mac & ~(0xffffULL << 48);
    memcpy(key_.data() + ofs, (char*)&mac_, sizeof(mac_));
}

void Device::setIp4(quint32 address, int prefixLength, quint32 gateway)
{
    ip4_ = address;
    ip4PrefixLength_ = prefixLength;
    ip4Gateway_ = gateway;
    hasIp4_ = true;

    // Precalculate our mask 'n subnet to avoid doing so at pkt rx/tx time
    ip4Mask_ = ~0 << (32 - ip4PrefixLength_);
    ip4Subnet_ = ip4_ & ip4Mask_;
}

void Device::setIp6(UInt128 address, int prefixLength, UInt128 gateway)
{
    ip6_ = address;
    ip6PrefixLength_ = prefixLength;
    ip6Gateway_ = gateway;
    hasIp6_ = true;

    // Precalculate our mask 'n subnet to avoid doing so at pkt rx/tx time
    ip6Mask_ = ~UInt128(0, 0) << (128 - ip6PrefixLength_);
    ip6Subnet_ = ip6_ & ip6Mask_;
}

void Device::getConfig(OstEmul::Device *deviceConfig)
{
    for (int i = 0; i < numVlanTags_; i++)
        deviceConfig->add_vlan(vlan_[i]);
    deviceConfig->set_mac(mac_);

    if (hasIp4_) {
        deviceConfig->set_ip4(ip4_);
        deviceConfig->set_ip4_prefix_length(ip4PrefixLength_);
        deviceConfig->set_ip4_default_gateway(ip4Gateway_);
    }

    if (hasIp6_) {
        deviceConfig->mutable_ip6()->set_hi(ip6_.hi64());
        deviceConfig->mutable_ip6()->set_lo(ip6_.lo64());
        deviceConfig->set_ip6_prefix_length(ip6PrefixLength_);
        deviceConfig->mutable_ip6_default_gateway()->set_hi(ip6Gateway_.hi64());
        deviceConfig->mutable_ip6_default_gateway()->set_lo(ip6Gateway_.lo64());
    }
}

QString Device::config()
{
    QString config;

    for (int i = 0; i < numVlanTags_; i++) {
        config.append(i == 0 ? "vlans=" : "|");
        config.append(
                (vlan_[i] >> 16) != kVlanTpid ?
                QString("0x%1-%2")
                    .arg(vlan_[i] >> 16, 4, kBaseHex, QChar('0'))
                    .arg(vlan_[i] & 0xFFFF) :
                QString("%1")
                    .arg(vlan_[i] & 0xFFFF));
    }

    config.append(QString(" mac=%1")
            .arg(mac_, 12, kBaseHex, QChar('0')));
    if (hasIp4_)
        config.append(QString(" ip4=%1/%2")
                .arg(QHostAddress(ip4_).toString())
                .arg(ip4PrefixLength_));
    if (hasIp6_)
        config.append(QString(" ip6=%1/%2")
                .arg(QHostAddress(ip6_.toArray()).toString())
                .arg(ip6PrefixLength_));

    return config;
}

DeviceKey Device::key()
{
    return key_;
}

void Device::clearKey()
{
    key_.fill(0, kMaxVlan * sizeof(quint16) + sizeof(quint64));
}

int Device::encapSize()
{
    // ethernet header + vlans
    int size = 14 + 4*numVlanTags_;

    return size;
}

void Device::encap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type)
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

// We expect pktBuf to point to EthType on entry
void Device::receivePacket(PacketBuffer *pktBuf)
{
    quint16 ethType = qFromBigEndian<quint16>(pktBuf->data());
    pktBuf->pull(2);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    switch(ethType)
    {
    case kEthTypeArp: // ARP
        if (hasIp4_)
            receiveArp(pktBuf);
        break;

    case kEthTypeIp4: // IPv4
        if (hasIp4_)
            receiveIp4(pktBuf);
        break;

    case kEthTypeIp6: // IPv6
        if (hasIp6_)
            receiveIp6(pktBuf);
        break;

    default:
        break;
    }
    // FIXME: temporary hack till DeviceManager clones pbufs
    pktBuf->push(2);
}

void Device::transmitPacket(PacketBuffer *pktBuf)
{
    deviceManager_->transmitPacket(pktBuf);
}

void Device::resolveGateway()
{
    if (hasIp4_)
        sendArpRequest(ip4Gateway_);

    if (hasIp6_)
        sendNeighborSolicit(ip6Gateway_);
}

void Device::clearNeighbors(Device::NeighborSet set)
{
    QMutableHashIterator<quint32, quint64> arpIter(arpTable_);
    QMutableHashIterator<UInt128, quint64> ndpIter(ndpTable_);

    switch (set) {
    case kAllNeighbors:
        arpTable_.clear();
        ndpTable_.clear();
        break;

    case kUnresolvedNeighbors:
        while (arpIter.hasNext()) {
            arpIter.next();
            if (arpIter.value() == 0)
                arpIter.remove();
        }

        while (ndpIter.hasNext()) {
            ndpIter.next();
            if (ndpIter.value() == 0)
                ndpIter.remove();
        }
        break;
    default:
        Q_ASSERT(false); // Unreachable!
    }
}

// Resolve the Neighbor IP address for this to-be-transmitted pktBuf
// We expect pktBuf to point to EthType on entry
void Device::resolveNeighbor(PacketBuffer *pktBuf)
{
    quint16 ethType = qFromBigEndian<quint16>(pktBuf->data());
    pktBuf->pull(2);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    switch(ethType)
    {
    case kEthTypeIp4: // IPv4
        if (hasIp4_)
            sendArpRequest(pktBuf);
        break;

    case kEthTypeIp6: // IPv6
        if (hasIp6_)
            sendNeighborSolicit(pktBuf);
        break;

    default:
        break;
    }
    // FIXME: temporary hack till DeviceManager clones pbufs
    pktBuf->push(2);
}

// Append this device's neighbors to the list
void Device::getNeighbors(OstEmul::DeviceNeighborList *neighbors)
{
    QList<quint32> ip4List = arpTable_.keys();
    QList<UInt128> ip6List = ndpTable_.keys();
    QList<quint64> macList;

    macList = arpTable_.values();
    Q_ASSERT(ip4List.size() == macList.size());

    for (int i = 0; i < ip4List.size(); i++) {
        OstEmul::ArpEntry *arp = neighbors->add_arp();
        arp->set_ip4(ip4List.at(i));
        arp->set_mac(macList.at(i));
    }

    macList = ndpTable_.values();
    Q_ASSERT(ip6List.size() == macList.size());

    for (int i = 0; i < ip6List.size(); i++) {
        OstEmul::NdpEntry *ndp = neighbors->add_ndp();
        ndp->mutable_ip6()->set_hi(ip6List.at(i).hi64());
        ndp->mutable_ip6()->set_lo(ip6List.at(i).lo64());
        ndp->set_mac(macList.at(i));
    }
}

// Are we the source of the given packet?
// We expect pktBuf to point to EthType on entry
bool Device::isOrigin(const PacketBuffer *pktBuf)
{
    const uchar *pktData = pktBuf->data();
    quint16 ethType = qFromBigEndian<quint16>(pktData);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);
    pktData += 2;

    // We know only about IP packets - adjust for ethType length (2 bytes)
    // when checking that we have a complete IP header
    if ((ethType == kEthTypeIp4) && hasIp4_) { // IPv4
        int ipHdrLen = (pktData[0] & 0x0F) << 2;
        quint32 srcIp;

        if (pktBuf->length() < (ipHdrLen+2)) {
            qDebug("incomplete IPv4 header: expected %d, actual %d",
                    ipHdrLen, pktBuf->length());
            return false;
        }

        srcIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 8);
        qDebug("%s: pktSrcIp/selfIp = 0x%x/0x%x", __FUNCTION__, srcIp, ip4_);
        return (srcIp == ip4_);
    }
    else if ((ethType == kEthTypeIp6) && hasIp6_) { // IPv6
        UInt128 srcIp;
        if (pktBuf->length() < (kIp6HdrLen+2)) {
            qDebug("incomplete IPv6 header: expected %d, actual %d",
                    kIp6HdrLen, pktBuf->length()-2);
            return false;
        }

        srcIp = qFromBigEndian<UInt128>(pktData + 8);
        qDebug("%s: pktSrcIp6/selfIp6 = %llx-%llx/%llx-%llx", __FUNCTION__,
                srcIp.hi64(), srcIp.lo64(), ip6_.hi64(), ip6_.lo64());
        return (srcIp == ip6_);
    }

    return false;
}

// Return the mac address corresponding to the dstIp of the given packet
// We expect pktBuf to point to EthType on entry
quint64 Device::neighborMac(const PacketBuffer *pktBuf)
{
    const uchar *pktData = pktBuf->data();
    quint16 ethType = qFromBigEndian<quint16>(pktData);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);
    pktData += 2;

    // We know only about IP packets
    if ((ethType == kEthTypeIp4) && hasIp4_) { // IPv4
        int ipHdrLen = (pktData[0] & 0x0F) << 2;
        quint32 dstIp, tgtIp;

        if (pktBuf->length() < ipHdrLen) {
            qDebug("incomplete IPv4 header: expected %d, actual %d",
                    ipHdrLen, pktBuf->length());
            return false;
        }

        dstIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 4);
        if ((dstIp & 0xF0000000) == 0xE0000000) { // Mcast IP?
            qDebug("mcast dst %x", dstIp);
            return (quint64(0x01005e) << 24) | (dstIp & 0x7FFFFF);
        }
        tgtIp = ((dstIp & ip4Mask_) == ip4Subnet_) ? dstIp : ip4Gateway_;

        return arpTable_.value(tgtIp);
    }
    else if ((ethType == kEthTypeIp6) && hasIp6_) { // IPv6
        UInt128 dstIp, tgtIp;

        if (pktBuf->length() < (kIp6HdrLen+2)) {
            qDebug("incomplete IPv6 header: expected %d, actual %d",
                    kIp6HdrLen, pktBuf->length()-2);
            return false;
        }

        dstIp = qFromBigEndian<UInt128>(pktData + 24);
        if (dstIp.toArray()[0] == 0xFF) { // Mcast IP?
            qDebug("mcast dst %s",
                    qPrintable(QHostAddress(dstIp.toArray()).toString()));
            return (quint64(0x3333) << 32) | (dstIp.lo64() & 0xFFFFFFFF);
        }
        tgtIp = ((dstIp & ip6Mask_) == ip6Subnet_) ? dstIp : ip6Gateway_;

        return ndpTable_.value(tgtIp);
    }

    return false;
}

//
// Private Methods
//
/*
 * ---------------------------------------------------------
 * IPv4 related private methods
 * ---------------------------------------------------------
 */
void Device::receiveArp(PacketBuffer *pktBuf)
{
    PacketBuffer *rspPkt;
    uchar *pktData = pktBuf->data();
    int offset = 0;
    quint16 hwType, protoType;
    quint8 hwAddrLen, protoAddrLen;
    quint16 opCode;
    quint64 srcMac, tgtMac;
    quint32 srcIp, tgtIp;

    // Extract tgtIp first to check quickly if this packet is for us or not
    tgtIp = qFromBigEndian<quint32>(pktData + 24);
    if (tgtIp != ip4_) {
        qDebug("tgtIp %s is not me %s",
                qPrintable(QHostAddress(tgtIp).toString()),
                qPrintable(QHostAddress(ip4_).toString()));
        return;
    }

    // Extract annd verify ARP packet contents
    hwType = qFromBigEndian<quint16>(pktData + offset);
    offset += 2;
    if (hwType != 1) // Mac
        goto _invalid_exit;

    protoType = qFromBigEndian<quint16>(pktData + offset);
    offset += 2;
    if (protoType != kEthTypeIp4) // IPv4
        goto _invalid_exit;

    hwAddrLen = pktData[offset];
    offset += 1;
    if (hwAddrLen != 6)
        goto _invalid_exit;

    protoAddrLen = pktData[offset];
    offset += 1;
    if (protoAddrLen != 4)
        goto _invalid_exit;

    opCode = qFromBigEndian<quint16>(pktData + offset);
    offset += 2;

    srcMac = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;
    srcMac = (srcMac << 16) | qFromBigEndian<quint16>(pktData + offset);
    offset += 2;

    srcIp = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;

    tgtMac = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;
    tgtMac = (tgtMac << 16) | qFromBigEndian<quint16>(pktData + offset);
    offset += 2;

    switch (opCode)
    {
    case 1:  // ARP Request
        arpTable_.insert(srcIp, srcMac);

        rspPkt = new PacketBuffer;
        rspPkt->reserve(encapSize());
        pktData = rspPkt->put(28);
        if (pktData) {
            // HTYP, PTYP
            *(quint32*)(pktData   ) = qToBigEndian(quint32(0x00010800));
            // HLEN, PLEN, OPER
            *(quint32*)(pktData+ 4) = qToBigEndian(quint32(0x06040002));
            // Source H/W Addr, Proto Addr
            *(quint32*)(pktData+ 8) = qToBigEndian(quint32(mac_ >> 16));
            *(quint16*)(pktData+12) = qToBigEndian(quint16(mac_ & 0xffff));
            *(quint32*)(pktData+14) = qToBigEndian(ip4_);
            // Target H/W Addr, Proto Addr
            *(quint32*)(pktData+18) = qToBigEndian(quint32(srcMac >> 16));
            *(quint16*)(pktData+22) = qToBigEndian(quint16(srcMac & 0xffff));
            *(quint32*)(pktData+24) = qToBigEndian(srcIp);
        }

        encap(rspPkt, srcMac, kEthTypeArp);
        transmitPacket(rspPkt);

        qDebug("Sent ARP Reply for srcIp/tgtIp=%s/%s",
                qPrintable(QHostAddress(srcIp).toString()),
                qPrintable(QHostAddress(tgtIp).toString()));
        break;
    case 2: // ARP Response
        arpTable_.insert(srcIp, srcMac);
        break;

    default:
        break;
    }

    return;

_invalid_exit:
    qWarning("Invalid ARP content");
    return;
}

// Send ARP request for the IPv4 packet in pktBuf
// pktBuf points to start of IP header
void Device::sendArpRequest(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int ipHdrLen = (pktData[0] & 0x0F) << 2;
    quint32 dstIp, tgtIp;

    if (pktBuf->length() < ipHdrLen) {
        qDebug("incomplete IPv4 header: expected %d, actual %d",
                ipHdrLen, pktBuf->length());
        return;
    }

    dstIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 4);

    tgtIp = ((dstIp & ip4Mask_) == ip4Subnet_) ? dstIp : ip4Gateway_;

    sendArpRequest(tgtIp);

}

void Device::sendArpRequest(quint32 tgtIp)
{
    quint32 srcIp = ip4_;
    PacketBuffer *reqPkt;
    uchar *pktData;

    // Validate target IP
    if (!tgtIp)
        return;

    // This function will be called once per unique stream - which
    // may all have the same dst IP; even if dst IP are different the
    // gateway for the different dst IP may all be same. However,
    // we don't want to send duplicate ARP requests, so we check
    // if the tgtIP is already in the cache (resolved or unresolved)
    // and if so, we don't resend it
    if (arpTable_.contains(tgtIp))
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

    encap(reqPkt, kBcastMac, kEthTypeArp);
    transmitPacket(reqPkt);
    arpTable_.insert(tgtIp, 0);

    qDebug("Sent ARP Request for srcIp/tgtIp=%s/%s",
            qPrintable(QHostAddress(srcIp).toString()),
            qPrintable(QHostAddress(tgtIp).toString()));
}

void Device::receiveIp4(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    uchar ipProto;
    quint32 dstIp;

    if (pktData[0] != 0x45) {
        qDebug("%s: Unsupported IP version or options (%02x) ", __FUNCTION__,
                pktData[0]);
        goto _invalid_exit;
    }

    if (pktBuf->length() < 20) {
        qDebug("incomplete IPv4 header: expected 20, actual %d",
                pktBuf->length());
        goto _invalid_exit;
    }

    // XXX: We don't verify IP Header checksum

    dstIp = qFromBigEndian<quint32>(pktData + 16);
    if (dstIp != ip4_) {
        qDebug("%s: dstIp %x is not me (%x)", __FUNCTION__, dstIp, ip4_);
        goto _invalid_exit;
    }

    ipProto = pktData[9];
    qDebug("%s: ipProto = %d", __FUNCTION__, ipProto);
    switch (ipProto) {
    case 1: // ICMP
        pktBuf->pull(20);
        receiveIcmp4(pktBuf);
        break;
    default:
        qWarning("%s: Unsupported ipProto %d", __FUNCTION__, ipProto);
        break;
    }

_invalid_exit:
    return;
}

// This function assumes we are replying back to the same IP
// that originally sent us the packet and therefore we can reuse the
// ingress packet for egress; in other words, it assumes the
// original IP header is intact and will just reuse it after
// minimal modifications
void Device::sendIp4Reply(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->push(20);
    uchar origTtl = pktData[8];
    uchar ipProto = pktData[9];
    quint32 srcIp, dstIp, tgtIp;
    quint32 sum;

    // Swap src/dst IP addresses
    dstIp = qFromBigEndian<quint32>(pktData + 12); // srcIp in original pkt
    srcIp = qFromBigEndian<quint32>(pktData + 16); // dstIp in original pkt

    tgtIp = ((dstIp & ip4Mask_) == ip4Subnet_) ? dstIp : ip4Gateway_;

    if (!arpTable_.contains(tgtIp)) {
        qWarning("%s: mac not found for %s; unable to send IPv4 packet",
                __FUNCTION__, qPrintable(QHostAddress(tgtIp).toString()));
        return;
    }

    *(quint32*)(pktData + 12) = qToBigEndian(srcIp);
    *(quint32*)(pktData + 16) = qToBigEndian(dstIp);

    // Reset TTL
    pktData[8] = 64;

    // Incremental checksum update (RFC 1624 [Eqn.3])
    // HC' = ~(~HC + ~m + m')
    sum =  quint16(~qFromBigEndian<quint16>(pktData + 10)); // old cksum
    sum += quint16(~quint16(origTtl << 8 | ipProto)); // old value
    sum += quint16(pktData[8] << 8 | ipProto); // new value
    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    *(quint16*)(pktData + 10) = qToBigEndian(quint16(~sum));

    encap(pktBuf, arpTable_.value(tgtIp), kEthTypeIp4);
    transmitPacket(pktBuf);
}

void Device::receiveIcmp4(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    quint32 sum;

    // XXX: We don't verify icmp checksum

    // We handle only ping request
    if (pktData[0] != 8) { // Echo Request
        qDebug("%s: Ignoring non echo request (%d)", __FUNCTION__, pktData[0]);
        return;
    }

    pktData[0] = 0; // Echo Reply

    // Incremental checksum update (RFC 1624 [Eqn.3])
    // HC' = ~(~HC + ~m + m')
    sum =  quint16(~qFromBigEndian<quint16>(pktData + 2)); // old cksum
    sum += quint16(~quint16(8 << 8 | pktData[1])); // old value
    sum += quint16(0 << 8 | pktData[1]); // new value
    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    *(quint16*)(pktData + 2) = qToBigEndian(quint16(~sum));

    sendIp4Reply(pktBuf);
    qDebug("Sent ICMP Echo Reply");
}

/*
 * ---------------------------------------------------------
 * IPV6 related private methods
 * ---------------------------------------------------------
 */

void Device::receiveIp6(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    uchar ipProto;
    UInt128 dstIp;

    if ((pktData[0] & 0xF0) != 0x60) {
        qDebug("%s: Unsupported IP version (%02x) ", __FUNCTION__,
                pktData[0]);
        goto _invalid_exit;
    }

    if (pktBuf->length() < kIp6HdrLen) {
        qDebug("incomplete IPv6 header: expected %d, actual %d",
                kIp6HdrLen, pktBuf->length());
        goto _invalid_exit;
    }

    // FIXME: check for specific mcast address(es) instead of any mcast?
    dstIp = qFromBigEndian<UInt128>(pktData + 24);
    if (!isIp6Mcast(dstIp) && (dstIp != ip6_)) {
        qDebug("%s: dstIp %s is not me (%s)", __FUNCTION__,
                qPrintable(QHostAddress(dstIp.toArray()).toString()),
                qPrintable(QHostAddress(ip6_.toArray()).toString()));
        goto _invalid_exit;
    }

    ipProto = pktData[6];
    switch (ipProto) {
    case kIpProtoIcmp6:
        pktBuf->pull(kIp6HdrLen);
        receiveIcmp6(pktBuf);
        break;
    default:
        break;
    }

_invalid_exit:
    return;
}

// pktBuf should point to start of IP payload
bool Device::sendIp6(PacketBuffer *pktBuf, UInt128 dstIp, quint8 protocol)
{
    int payloadLen = pktBuf->length();
    uchar *p = pktBuf->push(kIp6HdrLen);
    quint64 dstMac;

    if (!p) {
        qWarning("%s: failed to push %d bytes [0x%p, 0x%p]", __FUNCTION__,
                kIp6HdrLen, pktBuf->head(), pktBuf->data());
        goto _error_exit;
    }

    // In case of mcast, derive dstMac
    if ((dstIp.hi64() >> 56) == 0xff)
        dstMac = (quint64(0x3333) << 32) | (dstIp.lo64() & 0xffffffff);
    else {
        UInt128 tgtIp = ((dstIp & ip6Mask_) == ip6Subnet_)? dstIp : ip6Gateway_;
        dstMac = ndpTable_.value(tgtIp);
    }

    if (!dstMac) {
        qWarning("%s: mac not found for %s; unable to send IPv6 packet",
                __FUNCTION__,
                qPrintable(QHostAddress(dstIp.toArray()).toString()));
        goto _error_exit;
    }

    // Ver(4), TrfClass(8), FlowLabel(8)
    *(quint32*)(p   ) = qToBigEndian(quint32(0x60000000));
    *(quint16*)(p+ 4) = qToBigEndian(quint16(payloadLen));
    p[6] = protocol;
    p[7] = 255; // HopLimit
    memcpy(p+ 8,  ip6_.toArray(), 16); // Source IP
    memcpy(p+24, dstIp.toArray(), 16); // Destination IP

    // FIXME: both these functions should return success/failure
    encap(pktBuf, dstMac, kEthTypeIp6);
    transmitPacket(pktBuf);

    return true;

_error_exit:
    return false;
}

// This function assumes we are replying back to the same IP
// that originally sent us the packet and therefore we can reuse the
// ingress packet for egress; in other words, it assumes the
// original IP header is intact and will just reuse it after
// minimal modifications
void Device::sendIp6Reply(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->push(kIp6HdrLen);
    UInt128 srcIp, dstIp, tgtIp;

    // Swap src/dst IP addresses
    dstIp = qFromBigEndian<UInt128>(pktData +  8); // srcIp in original pkt
    srcIp = qFromBigEndian<UInt128>(pktData + 24); // dstIp in original pkt

    tgtIp = ((dstIp & ip6Mask_) == ip6Subnet_) ? dstIp : ip6Gateway_;
    if (!ndpTable_.contains(tgtIp)) {
        qWarning("%s: mac not found for %s; unable to send IPv6 packet",
                __FUNCTION__,
                qPrintable(QHostAddress(tgtIp.toArray()).toString()));
        return;
    }

    memcpy(pktData +  8, srcIp.toArray(), 16); // Source IP
    memcpy(pktData + 24, dstIp.toArray(), 16); // Destination IP

    // Reset TTL
    pktData[7] = 64;

    encap(pktBuf, ndpTable_.value(tgtIp), kEthTypeIp6);
    transmitPacket(pktBuf);
}

void Device::receiveIcmp6(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    quint8 type = pktData[0];
    quint32 sum;

    // XXX: We don't verify icmp checksum

    switch (type) {
        case 128: // ICMPv6 Echo Request
            pktData[0] = 129; // Echo Reply

            // Incremental checksum update (RFC 1624 [Eqn.3])
            // HC' = ~(~HC + ~m + m')
            sum =  quint16(~qFromBigEndian<quint16>(pktData + 2)); // old cksum
            sum += quint16(~quint16(128 << 8 | pktData[1])); // old value
            sum += quint16(129 << 8 | pktData[1]); // new value
            while(sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);
            *(quint16*)(pktData + 2) = qToBigEndian(quint16(~sum));

            sendIp6Reply(pktBuf);
            qDebug("Sent ICMPv6 Echo Reply");
            break;

        case 135: // Neigh Solicit
        case 136: // Neigh Advt
            receiveNdp(pktBuf);
            break;
        default:
            break;
    }
}

void Device::receiveNdp(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    quint8 type  = pktData[0];
    int len = pktBuf->length();
    int minLen = 24 + (type == 136 ? 8 : 0); // NA should have the Target TLV

    if (len < minLen) {
        qDebug("%s: incomplete NS/NA header: expected %d, actual %d",
                __FUNCTION__, minLen, pktBuf->length());
        goto _invalid_exit;
    }

    switch (type)
    {
        case 135: { // Neigh Solicit
            // TODO: Validation as per RFC 4861
            sendNeighborAdvertisement(pktBuf);
            break;
        }
        case 136: { // Neigh Advt
            quint8 flags = pktData[4];
            const quint8 kSFlag = 0x40;
            const quint8 kOFlag = 0x20;
            UInt128 tgtIp = qFromBigEndian<UInt128>(pktData + 8);
            quint64 mac = ndpTable_.value(tgtIp);

            // Update NDP table only for solicited responses
            if (!(flags & kSFlag))
                break;

            if ((flags & kOFlag) || (mac == 0)) {
                // Check if we have a Target Link-Layer TLV
                if ((pktData[24] != 2) || (pktData[25] != 1))
                    goto _invalid_exit;
                mac = qFromBigEndian<quint32>(pktData + 26);
                mac = (mac << 16) | qFromBigEndian<quint16>(pktData + 30);
                ndpTable_.insert(tgtIp, mac);
            }
            break;
        }
    }
_invalid_exit:
    return;
}

// Send NS for the IPv6 packet in pktBuf
// caller is responsible to check that pktBuf originates from this device
// pktBuf should point to start of IP header
void Device::sendNeighborSolicit(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    UInt128 dstIp, tgtIp;

    if (pktBuf->length() < kIp6HdrLen) {
        qDebug("incomplete IPv6 header: expected %d, actual %d",
                kIp6HdrLen, pktBuf->length());
        return;
    }

    dstIp = qFromBigEndian<UInt128>(pktData + 24);

    tgtIp = ((dstIp & ip6Mask_) == ip6Subnet_) ? dstIp : ip6Gateway_;

    sendNeighborSolicit(tgtIp);
}

void Device::sendNeighborSolicit(UInt128 tgtIp)
{
    UInt128 dstIp, srcIp = ip6_;
    PacketBuffer *reqPkt;
    uchar *pktData;

    // Validate target IP
    if (tgtIp == UInt128(0, 0))
        return;

    // Do we already have a NDP entry (resolved or unresolved)?
    // If so, don't resend (see note in sendArpRequest())
    if (ndpTable_.contains(tgtIp))
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

    if (!sendIp6(reqPkt, dstIp , kIpProtoIcmp6))
        return;

    ndpTable_.insert(tgtIp, 0);

    qDebug("Sent NDP Request for srcIp/tgtIp=%s/%s",
            qPrintable(QHostAddress(srcIp.toArray()).toString()),
            qPrintable(QHostAddress(tgtIp.toArray()).toString()));
}

// Send NA for the NS packet in pktBuf
// pktBuf should point to start of ICMPv6 header
void Device::sendNeighborAdvertisement(PacketBuffer *pktBuf)
{
    PacketBuffer *naPkt;
    uchar *pktData = pktBuf->data();
    quint16 flags = 0x6000; // solicit = 1; overide = 1
    uchar *ip6Hdr;
    UInt128 tgtIp, srcIp;

    tgtIp = qFromBigEndian<UInt128>(pktData + 8);
    if (tgtIp != ip6_) {
        qDebug("%s: NS tgtIp %s is not us %s", __FUNCTION__,
                qPrintable(QHostAddress(tgtIp.toArray()).toString()),
                qPrintable(QHostAddress(ip6_.toArray()).toString()));
        ip6Hdr = pktBuf->push(kIp6HdrLen);
        return;
    }

    ip6Hdr = pktBuf->push(kIp6HdrLen);
    srcIp = qFromBigEndian<UInt128>(ip6Hdr + 8);

    if (srcIp == UInt128(0, 0)) {
        // reset solicit flag
        flags &= ~0x4000;
        // NA should be sent to All nodes address
        srcIp = UInt128(quint64(0xff02) << 48, quint64(1));
    }
    else if (pktBuf->length() >= 32) { // have TLVs?
        if ((pktData[24] == 0x01) && (pktData[25] == 0x01)) { // Source TLV
            quint64 mac;
            mac = qFromBigEndian<quint32>(pktData + 26);
            mac = (mac << 16) | qFromBigEndian<quint16>(pktData + 30);
            ndpTable_.insert(srcIp, mac);
        }
    }

    naPkt = new PacketBuffer;
    naPkt->reserve(encapSize() + kIp6HdrLen);
    pktData = naPkt->put(32);
    if (pktData) {
        // Calculate checksum first -
        // start with fixed fields in ICMP Header and IPv6 Pseudo Header ...
        quint32 sum = (0x8800 + flags + 0x0201) + (32 + kIpProtoIcmp6);

        // then variable fields from ICMP header ...
        sum += sumUInt128(tgtIp);
        sum += (mac_ >> 32) + ((mac_ >> 16) & 0xffff) + (mac_ & 0xffff);

        // and variable fields from IPv6 pseudo header
        sum += sumUInt128(ip6_);
        sum += sumUInt128(srcIp);

        while(sum >> 16)
            sum = (sum & 0xFFFF) + (sum >> 16);

        // Type, Code
        *(quint16*)(pktData   ) = qToBigEndian(quint16(0x8800));
        // Checksum
        *(quint16*)(pktData+ 2) = qToBigEndian(quint16(~sum));
        // Flags-Reserved
        *(quint32*)(pktData+ 4) = qToBigEndian(quint32(flags << 16));
        // Target IP
        memcpy(pktData+ 8, tgtIp.toArray(), 16);
        // Target Addr TLV + MacAddr
        *(quint16*)(pktData+24) = qToBigEndian(quint16(0x0201));
        *(quint32*)(pktData+26) = qToBigEndian(quint32(mac_ >> 16));
        *(quint16*)(pktData+30) = qToBigEndian(quint16(mac_ & 0xffff));
    }

    if (!sendIp6(naPkt, srcIp , kIpProtoIcmp6))
        return;

    qDebug("Sent Neigh Advt to dstIp for tgtIp=%s/%s",
            qPrintable(QHostAddress(srcIp.toArray()).toString()),
            qPrintable(QHostAddress(tgtIp.toArray()).toString()));
}

bool operator<(const DeviceKey &a1, const DeviceKey &a2)
{
    int i = 0;

    while (i < a1.size()) {
        if (uchar(a1.at(i)) < uchar(a2.at(i)))
            return true;
        if (uchar(a1.at(i)) > uchar(a2.at(i)))
            return false;
        i++;
    }

    return false;
}
