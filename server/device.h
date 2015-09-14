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

#include "../common/protocol.pb.h"

#include <QHash>

class DeviceManager;
class PacketBuffer;

class Device
{
public:
    Device(quint32 id, DeviceManager *deviceManager);
    ~Device();

    quint32 id();

    void protoDataCopyFrom(const OstProto::Device &device);
    void protoDataCopyInto(OstProto::Device &device) const;

    int encapSize();
    void encap(PacketBuffer *pktBuf, quint64 dstMac, quint16 type);

    // receivePacket() is a class method 'coz we don't have the target
    // device yet; transmitPacket() is always from a particular device
    void transmitPacket(PacketBuffer *pktBuf);
    static void receivePacket(PacketBuffer *pktBuf);

private: // methods
    // receiveArp() is a class method 'coz ARP request is broadcast, so
    // we can't identify the target device till we parse the ARP header
    static void receiveArp(Device *device, PacketBuffer *pktBuf);

    quint64 myMac();
    quint32 myIp4();

private: // data
    // Class data
    static QHash<quint64, Device*> macHash_;
    static QHash<quint32, Device*> ip4Hash_;

    DeviceManager *deviceManager_;
    OstProto::Device data_;
};

#endif

