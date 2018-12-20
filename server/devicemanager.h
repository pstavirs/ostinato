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

#include "device.h"

#include <QHash>
#include <QMap>
#include <QMultiHash>
#include <QtGlobal>

class AbstractPort;
class PacketBuffer;
namespace OstProto {
    class DeviceGroup;
};

class DeviceManager
{
public:
    DeviceManager(AbstractPort *parent = 0);
    ~DeviceManager();

    void createHostDevices();

    int deviceGroupCount();
    const OstProto::DeviceGroup* deviceGroupAtIndex(int index);
    const OstProto::DeviceGroup* deviceGroup(uint deviceGroupId);

    bool addDeviceGroup(uint deviceGroupId);
    bool deleteDeviceGroup(uint deviceGroupId);
    bool modifyDeviceGroup(const OstProto::DeviceGroup *deviceGroup);

    int deviceCount();
    void getDeviceList(OstProto::PortDeviceList *deviceList);

    void receivePacket(PacketBuffer *pktBuf);
    void transmitPacket(PacketBuffer *pktBuf);

    void resolveDeviceGateways();

    void clearDeviceNeighbors(Device::NeighborSet set = Device::kAllNeighbors);
    void resolveDeviceNeighbor(PacketBuffer *pktBuf);
    void getDeviceNeighbors(OstProto::PortNeighborList *neighborList);

    quint64 deviceMacAddress(PacketBuffer *pktBuf);
    quint64 neighborMacAddress(PacketBuffer *pktBuf);

private:
    enum Operation { kAdd, kDelete };

    Device* originDevice(PacketBuffer *pktBuf);
    void enumerateDevices(
            const OstProto::DeviceGroup *deviceGroup,
            Operation oper);
    bool insertDevice(DeviceKey key, Device *device);
    bool deleteDevice(DeviceKey key);

    AbstractPort *port_;

    QHash<uint, OstProto::DeviceGroup*> deviceGroupList_;
    QHash<DeviceKey, Device*> deviceList_; // fast access to devices
    QMap<DeviceKey, Device*> sortedDeviceList_; // sorted access to devices
    QMultiHash<DeviceKey, Device*> bcastList_;
    QHash<quint16, uint> tpidList_; // Key: TPID, Value: RefCount

    QList<Device*> hostDeviceList_;
};

#endif

