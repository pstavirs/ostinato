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
const int kMaxVlan = 4;

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
    ip4_ = 0;
    ip4PrefixLength_ = 0;

    clearKey();
}

void Device::setVlan(int index, quint16 vlan)
{
    int ofs;

    if ((index < 0) || (index >= kMaxVlan)) {
        qWarning("%s: vlan index %d out of range (0 - %d)", __FUNCTION__,
                index, kMaxVlan - 1);
        return;
    }

    vlan_[index] = vlan;

    ofs = index * sizeof(quint16);
    key_[ofs]   = vlan >> 8;
    key_[ofs+1] = vlan & 0xff;

    if (index >= numVlanTags_)
        numVlanTags_ = index + 1;
}

void Device::setMac(quint64 mac)
{
    int ofs = kMaxVlan * sizeof(quint16);

    mac_ = mac;
    memcpy(key_.data() + ofs, (char*)&mac, sizeof(mac));
}

void Device::setIp4(quint32 address, int prefixLength)
{
    ip4_ = address;
    ip4PrefixLength_ = prefixLength;
}

QString Device::config()
{
    return QString("<vlans=%1/%2/%3/%4 mac=%5 ip4=%6/%7>")
        .arg(vlan_[0]).arg(vlan_[1]).arg(vlan_[2]).arg(vlan_[3])
        .arg(mac_, 12, kBaseHex, QChar('0'))
        .arg(QHostAddress(ip4_).toString())
        .arg(ip4PrefixLength_);
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
        *(quint16*)(p + ofs) =  qToBigEndian(quint32((0x8100 << 16)|vlan_[i]));
        ofs += 4;
    }
    *(quint16*)(p + ofs) =  qToBigEndian(type);
    ofs += 2;

    Q_ASSERT(ofs == encapSize());

_exit:
    return;
}

void Device::transmitPacket(PacketBuffer *pktBuf)
{
    deviceManager_->transmitPacket(pktBuf);
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
    case 0x86dd: // IPv6
    default:
        break;
    }
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
    default:
        break;
    }

    return;

_invalid_exit:
    qWarning("Invalid ARP content");
    return;
}
