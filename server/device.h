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

#ifndef _DEVICE_H
#define _DEVICE_H

#include "../common/emulproto.pb.h"
#include "../common/protocol.pb.h"

#include <QByteArray>
#include <QHash>

class DeviceManager;
class PacketBuffer;

typedef QByteArray DeviceKey;

class Device
{
public:
    Device(DeviceManager *deviceManager);

    void setVlan(int index, quint16 vlan);
    quint64 mac();
    void setMac(quint64 mac);
    void setIp4(quint32 address, int prefixLength, quint32 gateway);
    void getConfig(OstEmul::Device *deviceConfig);
    QString config();

    DeviceKey key();
    void clearKey();

    int encapSize();
    void encap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type);

    void receivePacket(PacketBuffer *pktBuf);
    void transmitPacket(PacketBuffer *pktBuf);

    void clearNeighbors();
    void resolveNeighbor(PacketBuffer *pktBuf);
    void getNeighbors(OstEmul::DeviceNeighborList *neighbors);

    bool isOrigin(const PacketBuffer *pktBuf);
    quint64 neighborMac(const PacketBuffer *pktBuf);

private: // methods
    void receiveArp(PacketBuffer *pktBuf);
    void sendArpRequest(PacketBuffer *pktBuf);

private: // data
    static const int kMaxVlan = 4;

    DeviceManager *deviceManager_;

    int numVlanTags_;
    quint16 vlan_[kMaxVlan]; // FIXME: vlan tpid
    quint64 mac_;
    quint32 ip4_;
    int ip4PrefixLength_;
    quint32 ip4Gateway_;

    DeviceKey key_;

    QHash<quint32, quint64> arpTable;
};

#endif
