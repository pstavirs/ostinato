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

#include <qendian.h>

QHash<quint64, Device*> Device::macHash_;
QHash<quint32, Device*> Device::ip4Hash_;

Device::Device(quint32 id, DeviceManager *deviceManager)
{
    deviceManager_ = deviceManager;
    data_.mutable_device_id()->set_id(id);
    // FIXME: choose a better default mac address
    data_.MutableExtension(OstEmul::mac)->set_addr(0x001122330000ULL + id);

    Device::macHash_.insert(myMac(), this);
}

Device::~Device()
{
    macHash_.remove(myMac());
    ip4Hash_.remove(myIp4());
}

quint32 Device::id()
{
    return data_.device_id().id();
}

void Device::protoDataCopyFrom(const OstProto::Device &device)
{
    quint64 oldMac, newMac;
    quint32 oldIp4, newIp4;

    // Save old mac and ip before updating the device data
    oldMac = myMac();
    oldIp4 = myIp4();

    data_.CopyFrom(device);

    // Get new mac and ip for comparison
    newMac = myMac();
    newIp4 = myIp4();

    // Update MacHash if mac has changed
    if (newMac != oldMac) {
        macHash_.remove(oldMac);
        macHash_.insert(newMac, this);
    }

    // Update Ip4Hash if ip4 has changed
    if (newIp4 != oldIp4) {
        ip4Hash_.remove(oldIp4);
        ip4Hash_.insert(newIp4, this);
    }
}

void Device::protoDataCopyInto(OstProto::Device &device) const
{
    device.CopyFrom(data_);
}

int Device::encapSize()
{
    int size = 14; // ethernet header size

    if (data_.HasExtension(OstEmul::vlan))
        size += 4 * data_.GetExtension(OstEmul::vlan).vlan_stack_size();

    return size;
}

void Device::encap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type)
{
    quint64 srcMac = myMac();
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
    // TODO: Vlan Encap
    *(quint16*)(p + 12) =  qToBigEndian(type);

_exit:
    return;
}

//
// Private Methods
//
quint64 Device::myMac()
{
    if (data_.HasExtension(OstEmul::mac))
        return data_.GetExtension(OstEmul::mac).addr();

    return 0;
}

quint32 Device::myIp4()
{
    if (data_.HasExtension(OstEmul::ip4))
        return data_.GetExtension(OstEmul::ip4).addr();

    return 0; // FIXME: how to indicate that we don't have a IP?
}

void Device::transmitPacket(PacketBuffer *pktBuf)
{
    deviceManager_->transmitPacket(pktBuf);
}

void Device::receivePacket(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int offset = 0;
    Device *device;
    const quint64 bcastMac = 0xffffffffffffULL;
    quint64 dstMac;
    quint16 ethType;

    // We assume pkt is ethernet
    // TODO: extend for other link layer types

    // Extract dstMac
    dstMac = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;
    dstMac = (dstMac << 16) | qFromBigEndian<quint16>(pktData + offset);
    offset += 2;

    // Skip srcMac - don't care
    offset += 6;
    qDebug("dstMac %llx", dstMac);

    // Is it destined for us?
    device = macHash_.value(dstMac);
    if (!device && (dstMac != bcastMac)) {
        qDebug("%s: dstMac %llx is not us", __FUNCTION__, dstMac);
        goto _exit;
    }

    ethType = qFromBigEndian<quint16>(pktData + offset);
    offset += 2;
    qDebug("%s: ethType 0x%x", __FUNCTION__, ethType);
    
    switch(ethType)
    {
    case 0x0806: // ARP
        pktBuf->pull(offset);
        receiveArp(device, pktBuf);
        break;

    case 0x8100: // VLAN
    case 0x0800: // IPv4
    case 0x86dd: // IPv6
    default:
        break;
    }

_exit:
    return;
}

void Device::receiveArp(Device *device, PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int offset = 0;
    quint16 hwType, protoType;
    quint8 hwAddrLen, protoAddrLen;
    quint16 opCode;
    quint64 srcMac, tgtMac;
    quint32 srcIp, tgtIp;

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

    tgtIp = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;

    switch (opCode)
    {
    case 1: // ARP Request
        if (!device)
            device = ip4Hash_.value(tgtIp);
        if (device->myIp4() == tgtIp) {
            PacketBuffer *pktBuf = new PacketBuffer;
            uchar *p;

            pktBuf->reserve(device->encapSize()); 
            p = pktBuf->put(28); // FIXME: hardcoding
            if (p) {
                // HTYP, PTYP
                *(quint32*)(p   ) = qToBigEndian(quint32(0x00010800));
                // HLEN, PLEN, OPER
                *(quint32*)(p+ 4) = qToBigEndian(quint32(0x06040002));
                // Source H/W Addr, Proto Addr
                *(quint32*)(p+ 8) = qToBigEndian(
                                        quint32(device->myMac() >> 16));
                *(quint16*)(p+12) = qToBigEndian(
                                        quint16(device->myMac() & 0xffff));
                *(quint32*)(p+14) = qToBigEndian(tgtIp);
                // Target H/W Addr, Proto Addr
                *(quint32*)(p+18) = qToBigEndian(quint32(srcMac >> 16));
                *(quint16*)(p+22) = qToBigEndian(quint16(srcMac & 0xffff));
                *(quint32*)(p+24) = qToBigEndian(srcIp);
            }

            device->encap(pktBuf, srcMac, 0x0806);
            device->transmitPacket(pktBuf);

            qDebug("Sent ARP Reply for srcIp/tgtIp=0x%x/0x%x",
                    srcIp, tgtIp);
        }
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
