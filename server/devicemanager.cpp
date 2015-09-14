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

#include "devicemanager.h"

#include "abstractport.h"
#include "device.h"
#include "packetbuffer.h"

#include <qendian.h>

// FIXME: add lock to protect deviceList_ operations?

DeviceManager::DeviceManager(AbstractPort *parent)
{
    port_ = parent;
}

DeviceManager::~DeviceManager()
{
    foreach(Device *dev, deviceList_)
        delete dev;
}

int DeviceManager::deviceCount()
{
    return deviceList_.size();
}

Device* DeviceManager::deviceAtIndex(int index)
{
    if ((index < 0) || (index >= deviceCount())) {
        qWarning("%s: index %d out of range (max %d)", __FUNCTION__,
                index, deviceCount());
        return NULL;
    }

    return deviceList_.value(deviceList_.uniqueKeys().value(index));
}

Device* DeviceManager::device(uint deviceId)
{
    return deviceList_.value(deviceId);
}

bool DeviceManager::addDevice(uint deviceId)
{
    Device *device;

    if (deviceList_.contains(deviceId)) {
        qWarning("%s: device id %u already exists", __FUNCTION__, deviceId);
        return false;
    }

    device = new Device(deviceId, this);
    deviceList_.insert(deviceId, device);

    if ((deviceCount() == 1) && port_)
        port_->startDeviceEmulation();

    return true;
}

bool DeviceManager::deleteDevice(uint deviceId)
{
    if (!deviceList_.contains(deviceId)) {
        qWarning("%s: device id %u does not exist", __FUNCTION__, deviceId);
        return false;
    }

    delete deviceList_.take(deviceId);

    if ((deviceCount() == 0) && port_)
        port_->stopDeviceEmulation();

    return true;
}

bool DeviceManager::modifyDevice(const OstProto::Device *device)
{
    quint32 id = device->device_id().id();
    Device *myDevice = deviceList_.value(id);
    if (!myDevice) {
        qWarning("%s: device id %u does not exist", __FUNCTION__, id);
        return false;
    }

    myDevice->protoDataCopyFrom(*device);

    return true;
}

void DeviceManager::receivePacket(PacketBuffer *pktBuf)
{
    Device::receivePacket(pktBuf);
}

void DeviceManager::transmitPacket(PacketBuffer *pktBuf)
{
    port_->sendEmulationPacket(pktBuf);
}
