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

#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include "../common/protocol.pb.h"

#include <QHash>
#include <QtGlobal>

class AbstractPort;
class Device;
class PacketBuffer;

class DeviceManager 
{
public:
    DeviceManager(AbstractPort *parent = 0);
    ~DeviceManager();

    int deviceCount();
    Device* deviceAtIndex(int index);
    Device* device(uint deviceId);

    bool addDevice(uint deviceId);
    bool deleteDevice(uint deviceId);
    bool modifyDevice(const OstProto::Device *device);

    void receivePacket(PacketBuffer *pktBuf);
    void transmitPacket(PacketBuffer *pktBuf);
private:
    AbstractPort *port_;
    QHash<uint, Device*> deviceList_;
};

#endif

