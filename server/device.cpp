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

/*
 * NOTE:
 * 1. Device Key is (VLANS + MAC) - is assumed to be unique for a device
 * 2. Device clients/users (viz. DeviceManager) should take care when
 *    setting params that change the key, if the key is used elsewhere
 *    (e.g. in a hash)
 */

Device::Device(DeviceManager *deviceManager)
{
    deviceManager_ = deviceManager;

    for (int i = 0; i < kMaxVlan; i++)
        vlan_[i] = 0;
    numVlanTags_ = 0;
    mac_ = 0;

    ip4_ = ip4Gateway_ = 0;
    ip4PrefixLength_ = 0;

    ip6_ = ip6Gateway_ = UInt128(0, 0);
    ip6PrefixLength_ = 0;

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

    mac_ = mac;
    memcpy(key_.data() + ofs, (char*)&mac, sizeof(mac));
}

void Device::setIp4(quint32 address, int prefixLength, quint32 gateway)
{
    ip4_ = address;
    ip4PrefixLength_ = prefixLength;
    ip4Gateway_ = gateway;
}

void Device::setIp6(UInt128 address, int prefixLength, UInt128 gateway)
{
    ip6_ = address;
    ip6PrefixLength_ = prefixLength;
    ip6Gateway_ = gateway;
}

void Device::getConfig(OstEmul::Device *deviceConfig)
{
    for (int i = 0; i < numVlanTags_; i++)
        deviceConfig->add_vlan(vlan_[i]);
    deviceConfig->set_mac(mac_);

    deviceConfig->set_ip4(ip4_);
    deviceConfig->set_ip4_prefix_length(ip4PrefixLength_);
    deviceConfig->set_ip4_default_gateway(ip4Gateway_);

#if 0 // FIXME
    deviceConfig->set_ip6(ip6_);
    deviceConfig->set_ip6_prefix_length(ip6PrefixLength_);
    deviceConfig->set_ip6_default_gateway(ip6Gateway_);
#endif
}

QString Device::config()
{
    return QString("<vlans=%1/%2/%3/%4 mac=%5 ip4=%6/%7 ip6=%8/%9>")
        .arg((vlan_[0] >> 16) != kVlanTpid ?
                QString("0x%1-%2")
                    .arg(vlan_[0] >> 16, 4, kBaseHex, QChar('0'))
                    .arg(vlan_[0] & 0xFFFF) :
                QString("%1")
                    .arg(vlan_[0] & 0xFFFF))
        .arg((vlan_[1] >> 16) != kVlanTpid ?
                QString("0x%1-%2")
                    .arg(vlan_[1] >> 16, 4, kBaseHex, QChar('0'))
                    .arg(vlan_[1] & 0xFFFF) :
                QString("%1")
                    .arg(vlan_[1] & 0xFFFF))
        .arg((vlan_[2] >> 16) != kVlanTpid ?
                QString("0x%1-%2")
                    .arg(vlan_[2] >> 16, 4, kBaseHex, QChar('0'))
                    .arg(vlan_[2] & 0xFFFF) :
                QString("%1")
                    .arg(vlan_[2] & 0xFFFF))
        .arg((vlan_[3] >> 16) != kVlanTpid ?
                QString("0x%1-%2")
                    .arg(vlan_[3] >> 16, 4, kBaseHex, QChar('0'))
                    .arg(vlan_[3] & 0xFFFF) :
                QString("%1")
                    .arg(vlan_[3] & 0xFFFF))
        .arg(mac_, 12, kBaseHex, QChar('0'))
        .arg(QHostAddress(ip4_).toString())
        .arg(ip4PrefixLength_)
        .arg(QHostAddress(ip6_.toArray()).toString())
        .arg(ip6PrefixLength_);
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
    case 0x0806: // ARP
        receiveArp(pktBuf);
        break;

    case 0x0800: // IPv4
        receiveIp4(pktBuf);
        break;

    case 0x86dd: // IPv6
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

void Device::clearNeighbors()
{
    arpTable.clear();
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
    case 0x0800: // IPv4
        sendArpRequest(pktBuf);
        break;

    case 0x86dd: // IPv6
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
    QList<quint32> ipList = arpTable.keys();
    QList<quint64> macList = arpTable.values();

    Q_ASSERT(ipList.size() == macList.size());

    for (int i = 0; i < ipList.size(); i++) {
        OstEmul::ArpEntry *arp = neighbors->add_arp();
        arp->set_ip4(ipList.at(i));
        arp->set_mac(macList.at(i));
    }
}

// We expect pktBuf to point to EthType on entry
bool Device::isOrigin(const PacketBuffer *pktBuf)
{
    const uchar *pktData = pktBuf->data();
    quint16 ethType = qFromBigEndian<quint16>(pktData);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);
    pktData += 2;

    // We know only about IP packets
    if (ethType == 0x0800) { // IPv4
        int ipHdrLen = (pktData[0] & 0x0F) << 2;
        quint32 srcIp;

        if (pktBuf->length() < ipHdrLen) {
            qDebug("incomplete IPv4 header: expected %d, actual %d",
                    ipHdrLen, pktBuf->length());
            return false;
        }

        srcIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 8);
        qDebug("%s: pktSrcIp/selfIp = 0x%x/0x%x", __FUNCTION__, srcIp, ip4_);
        return (srcIp == ip4_);
    }

    return false;
}

// We expect pktBuf to point to EthType on entry
quint64 Device::neighborMac(const PacketBuffer *pktBuf)
{
    const uchar *pktData = pktBuf->data();
    quint16 ethType = qFromBigEndian<quint16>(pktData);

    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);
    pktData += 2;

    // We know only about IP packets
    if (ethType == 0x0800) { // IPv4
        int ipHdrLen = (pktData[0] & 0x0F) << 2;
        quint32 dstIp, tgtIp, mask;

        if (pktBuf->length() < ipHdrLen) {
            qDebug("incomplete IPv4 header: expected %d, actual %d",
                    ipHdrLen, pktBuf->length());
            return false;
        }

        dstIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 4);
        mask =  ~0 << (32 - ip4PrefixLength_);
        qDebug("dst %x self %x mask %x", dstIp, ip4_, mask);
        tgtIp = ((dstIp & mask) == (ip4_ & mask)) ? dstIp : ip4Gateway_;

        return arpTable.value(tgtIp);
    }

    return false;
}

//
// Private Methods
//
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
    if (protoType != 0x0800) // IPv4
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
        arpTable.insert(srcIp, srcMac);

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

        encap(rspPkt, srcMac, 0x0806);
        transmitPacket(rspPkt);

        qDebug("Sent ARP Reply for srcIp/tgtIp=%s/%s",
                qPrintable(QHostAddress(srcIp).toString()),
                qPrintable(QHostAddress(tgtIp).toString()));
        break;
    case 2: // ARP Response
        arpTable.insert(srcIp, srcMac);
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
    PacketBuffer *reqPkt;
    uchar *pktData = pktBuf->data();
    int offset = 0;
    int ipHdrLen = (pktData[offset] & 0x0F) << 2;
    quint32 srcIp, dstIp, mask, tgtIp;

    if (pktBuf->length() < ipHdrLen) {
        qDebug("incomplete IPv4 header: expected %d, actual %d",
                ipHdrLen, pktBuf->length());
        return;
    }

    // Extract srcIp first to check quickly that this packet originates
    // from this device
    srcIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 8);
    if (srcIp != ip4_) {
        qDebug("%s: srcIp %s is not me %s", __FUNCTION__,
                qPrintable(QHostAddress(srcIp).toString()),
                qPrintable(QHostAddress(ip4_).toString()));
        return;
    }

    dstIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 4);

    mask =  ~0 << (32 - ip4PrefixLength_);
    qDebug("dst %x src %x mask %x", dstIp, srcIp, mask);
    tgtIp = ((dstIp & mask) == (srcIp & mask)) ? dstIp : ip4Gateway_;

    // Do we already have a ARP entry (resolved or unresolved)?
    // FIXME: do we need a timer to resend ARP for unresolved entries?
    if (arpTable.contains(tgtIp))
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

    encap(reqPkt, kBcastMac, 0x0806);
    transmitPacket(reqPkt);
    arpTable.insert(tgtIp, 0);

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
    switch (ipProto) {
    case 1: // ICMP
        pktBuf->pull(20);
        receiveIcmp4(pktBuf);
        break;
    default:
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
    quint32 srcIp, dstIp;
    quint32 sum;

    // Swap src/dst IP addresses
    dstIp = qFromBigEndian<quint32>(pktData + 12); // srcIp in original pkt
    srcIp = qFromBigEndian<quint32>(pktData + 16); // dstIp in original pkt

    if (!arpTable.contains(dstIp))
        return;

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

    encap(pktBuf, arpTable.value(dstIp), 0x0800);
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

// Send NS for the IPv6 packet in pktBuf
// pktBuf points to start of IP header
void Device::sendNeighborSolicit(PacketBuffer *pktBuf)
{
    // TODO
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
