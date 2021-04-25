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

#ifndef _EMUL_DEVICE_H
#define _EMUL_DEVICE_H

#include "device.h"

class EmulDevice: public Device
{
public:
    EmulDevice(DeviceManager *deviceManager);
    virtual ~EmulDevice() = default;

    void receivePacket(PacketBuffer *pktBuf);

    virtual void clearNeighbors(Device::NeighborSet set);
    virtual void getNeighbors(OstEmul::DeviceNeighborList *neighbors);

    bool isOrigin(const PacketBuffer *pktBuf);
    quint64 neighborMac(const PacketBuffer *pktBuf);

protected:
    virtual quint64 arpLookup(quint32 ip);
    virtual quint64 ndpLookup(UInt128 ip);
    virtual void sendArpRequest(quint32 tgtIp);
    virtual void sendNeighborSolicit(UInt128 tgtIp);

private: // methods
    int encapSize();
    void encap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type);

    void receiveArp(PacketBuffer *pktBuf);

    void receiveIp4(PacketBuffer *pktBuf);
    void sendIp4Reply(PacketBuffer *pktBuf);

    void receiveIcmp4(PacketBuffer *pktBuf);

    void receiveIp6(PacketBuffer *pktBuf);
    void sendIp6Reply(PacketBuffer *pktBuf);
    bool sendIp6(PacketBuffer *pktBuf, UInt128 dstIp, quint8 protocol);

    void receiveIcmp6(PacketBuffer *pktBuf);

    void receiveNdp(PacketBuffer *pktBuf);
    void sendNeighborAdvertisement(PacketBuffer *pktBuf);

private: // data
    QHash<quint32, quint64> arpTable_;
    QHash<UInt128, quint64> ndpTable_;
};

bool operator<(const DeviceKey &a1, const DeviceKey &a2);
#endif

