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
const quint16 kEthTypeIp4 = 0x0800;
const quint16 kEthTypeIp6 = 0x86dd;
const int kIp6HdrLen = 40;

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

/*
void Device::receivePacket(PacketBuffer *pktBuf)
{
    // XXX: Pure Virtual: Subclass should implement
    // We expect pktBuf to point to EthType on entry
}
*/

void Device::transmitPacket(PacketBuffer *pktBuf)
{
    deviceManager_->transmitPacket(pktBuf);
}

void Device::resolveGateway()
{
    if (hasIp4_ && !isResolved(ip4Gateway_))
        sendArpRequest(ip4Gateway_);

    if (hasIp6_ && !isResolved(ip6Gateway_))
        sendNeighborSolicit(ip6Gateway_);
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

/*
void Device::clearNeighbors(Device::NeighborSet set)
{
    // XXX: Pure virtual: Subclass should implement
}

// Append this device's neighbors to the list
void Device::getNeighbors(OstEmul::DeviceNeighborList* neighbors)
{
    // XXX: Subclass should implement
}
*/

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
    quint64 mac = 0;
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
            return mac;
        }

        dstIp = qFromBigEndian<quint32>(pktData + ipHdrLen - 4);
        if ((dstIp & 0xF0000000) == 0xE0000000) { // Mcast IP?
            mac = (quint64(0x01005e) << 24) | (dstIp & 0x7FFFFF);
            qDebug("mcast dst %08x, mac: %012llx", dstIp, mac);
        }
        else {
            tgtIp = ((dstIp & ip4Mask_) == ip4Subnet_) ? dstIp : ip4Gateway_;
            mac = arpLookup(tgtIp);
            qDebug("tgtIp: %08x, mac: %012llx", tgtIp, mac);
        }
    }
    else if ((ethType == kEthTypeIp6) && hasIp6_) { // IPv6
        UInt128 dstIp, tgtIp;

        if (pktBuf->length() < (kIp6HdrLen+2)) {
            qDebug("incomplete IPv6 header: expected %d, actual %d",
                    kIp6HdrLen, pktBuf->length()-2);
            return mac;
        }

        dstIp = qFromBigEndian<UInt128>(pktData + 24);
        if (dstIp.toArray()[0] == 0xFF) { // Mcast IP?
            mac = (quint64(0x3333) << 32) | (dstIp.lo64() & 0xFFFFFFFF);
            qDebug("mcast dst %s, mac: %012llx",
                    qPrintable(QHostAddress(dstIp.toArray()).toString()),
                    mac);
        }
        else {
            tgtIp = ((dstIp & ip6Mask_) == ip6Subnet_) ? dstIp : ip6Gateway_;
            mac = ndpLookup(tgtIp);
            qDebug("tgtIp %s, mac: %012llx",
                    qPrintable(QHostAddress(dstIp.toArray()).toString()),
                    mac);
        }
    }

    return mac;
}

/*
quint64 Device::arpLookup(quint32 ip)
{
    // XXX: Pure virtual: Subclass should implement
}

quint64 Device::ndpLookup(UInt128 ip)
{
    // XXX: Pure virtual: Subclass should implement
}
*/

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

    if (!isResolved(tgtIp))
        sendArpRequest(tgtIp);

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

    if (!isResolved(tgtIp))
        sendNeighborSolicit(tgtIp);
}

bool Device::isResolved(quint32 ip)
{
    return arpLookup(ip) > 0;
}

bool Device::isResolved(UInt128 ip)
{
    return ndpLookup(ip) > 0;
}

//
// Protected Methods
//
/*
void Device::sendArpRequest(quint32 tgtIp)
{
    // XXX: Pure virtual: Subclass should implement
}

void Device::sendNeighborSolicit(UInt128 tgtIp)
{
    // XXX: Pure virtual: Subclass should implement
}
*/

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
