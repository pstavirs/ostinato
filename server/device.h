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
#include "../common/uint128.h"

#include <QByteArray>
#include <QHash>
#include <QString>

class DeviceManager;
class PacketBuffer;

class DeviceKey: public QByteArray
{
};

class Device
{
public:
    static const quint16 kVlanTpid = 0x8100;

    enum NeighborSet {
        kAllNeighbors,
        kUnresolvedNeighbors
    };

public:
    Device(DeviceManager *deviceManager);
    virtual ~Device() = default;

    void setVlan(int index, quint16 vlan, quint16 tpid = kVlanTpid);
    quint64 mac();
    void setMac(quint64 mac);
    void setIp4(quint32 address, int prefixLength, quint32 gateway);
    void setIp6(UInt128 address, int prefixLength, UInt128 gateway);
    void getConfig(OstEmul::Device *deviceConfig);
    QString config();

    DeviceKey key();
    void clearKey();

    virtual void receivePacket(PacketBuffer *pktBuf) = 0;
    void transmitPacket(PacketBuffer *pktBuf);

    void resolveGateway();

    void resolveNeighbor(PacketBuffer *pktBuf);
    virtual void clearNeighbors(Device::NeighborSet set) = 0;
    virtual void getNeighbors(OstEmul::DeviceNeighborList *neighbors) = 0;

    bool isOrigin(const PacketBuffer *pktBuf);
    quint64 neighborMac(const PacketBuffer *pktBuf);

protected: // methods
    virtual quint64 arpLookup(quint32 ip) = 0;
    virtual quint64 ndpLookup(UInt128 ip) = 0;
    virtual void sendArpRequest(quint32 tgtIp) = 0;
    virtual void sendNeighborSolicit(UInt128 tgtIp) = 0;

protected: // data
    static const int kMaxVlan = 4;

    DeviceManager *deviceManager_;

    int numVlanTags_;
    quint32 vlan_[kMaxVlan];
    quint64 mac_;

    bool hasIp4_;
    quint32 ip4_;
    int ip4PrefixLength_;
    quint32 ip4Gateway_;
    quint32 ip4Mask_;
    quint32 ip4Subnet_;

    bool hasIp6_;
    UInt128 ip6_;
    int ip6PrefixLength_;
    UInt128 ip6Gateway_;
    UInt128 ip6Mask_;
    UInt128 ip6Subnet_;

    DeviceKey key_;

private: // methods
    void sendArpRequest(PacketBuffer *pktBuf);
    void sendNeighborSolicit(PacketBuffer *pktBuf);
    bool isResolved(quint32 ip);
    bool isResolved(UInt128 ip);

};

bool operator<(const DeviceKey &a1, const DeviceKey &a2);
#endif

