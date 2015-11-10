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

#include "../common/emulproto.pb.h"

#include <qendian.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

const quint64 kBcastMac = 0xffffffffffffULL;

// FIXME: add lock to protect deviceGroupList_ operations?

DeviceManager::DeviceManager(AbstractPort *parent)
{
    port_ = parent;
}

DeviceManager::~DeviceManager()
{
    foreach(Device *dev, deviceList_)
        delete dev;

    foreach(OstProto::DeviceGroup *devGrp, deviceGroupList_)
        delete devGrp;
}

int DeviceManager::deviceGroupCount()
{
    return deviceGroupList_.size();
}

const OstProto::DeviceGroup* DeviceManager::deviceGroupAtIndex(int index)
{
    if ((index < 0) || (index >= deviceGroupCount())) {
        qWarning("%s: index %d out of range (0 - %d)", __FUNCTION__,
                index, deviceGroupCount() - 1);
        return NULL;
    }

    // Sort List by 'id', get the id at 'index' and then corresponding devGrp
    return deviceGroupList_.value(deviceGroupList_.uniqueKeys().value(index));
}

const OstProto::DeviceGroup* DeviceManager::deviceGroup(uint deviceGroupId)
{
    return deviceGroupList_.value(deviceGroupId);
}

bool DeviceManager::addDeviceGroup(uint deviceGroupId)
{
    OstProto::DeviceGroup *deviceGroup;

    if (deviceGroupList_.contains(deviceGroupId)) {
        qWarning("%s: deviceGroup id %u already exists", __FUNCTION__, 
                deviceGroupId);
        return false;
    }

    deviceGroup = new OstProto::DeviceGroup;
    deviceGroup->mutable_device_group_id()->set_id(deviceGroupId);
    deviceGroupList_.insert(deviceGroupId, deviceGroup);

    enumerateDevices(deviceGroup, kAdd);

    // Start emulation when first device is added
    if ((deviceCount() == 1) && port_)
        port_->startDeviceEmulation();

    return true;
}

bool DeviceManager::deleteDeviceGroup(uint deviceGroupId)
{
    OstProto::DeviceGroup *deviceGroup;
    if (!deviceGroupList_.contains(deviceGroupId)) {
        qWarning("%s: deviceGroup id %u does not exist", __FUNCTION__, 
                deviceGroupId);
        return false;
    }

    deviceGroup = deviceGroupList_.take(deviceGroupId);
    enumerateDevices(deviceGroup, kDelete);
    delete deviceGroup;

    // Stop emulation if no devices remain
    if ((deviceCount() == 0) && port_)
        port_->stopDeviceEmulation();

    return true;
}

bool DeviceManager::modifyDeviceGroup(const OstProto::DeviceGroup *deviceGroup)
{
    quint32 id = deviceGroup->device_group_id().id();
    OstProto::DeviceGroup *myDeviceGroup = deviceGroupList_.value(id);
    if (!myDeviceGroup) {
        qWarning("%s: deviceGroup id %u does not exist", __FUNCTION__, id);
        return false;
    }

    enumerateDevices(myDeviceGroup, kDelete);
    myDeviceGroup->CopyFrom(*deviceGroup);
    enumerateDevices(myDeviceGroup, kAdd);

    return true;
}

int DeviceManager::deviceCount()
{
    return deviceList_.size();
}

void DeviceManager::receivePacket(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int offset = 0;
    Device dk(this);
    Device *device;
    quint64 dstMac;
    quint16 ethType;
    quint16 vlan;
    int idx = 0;

    // We assume pkt is ethernet
    // TODO: extend for other link layer types

    // FIXME: validate before extracting if the offset is within pktLen

    // Extract dstMac
    dstMac = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;
    dstMac = (dstMac << 16) | qFromBigEndian<quint16>(pktData + offset);
    dk.setMac(dstMac);
    offset += 2;

    // Skip srcMac - don't care
    offset += 6;

    qDebug("dstMac %012" PRIx64, dstMac);

_eth_type:
    // Extract EthType
    ethType = qFromBigEndian<quint16>(pktData + offset);
    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    if (ethType == 0x8100) {
        offset += 2;
        vlan = qFromBigEndian<quint16>(pktData + offset);
        dk.setVlan(idx++, vlan);
        offset += 2;
        qDebug("%s: idx: %d vlan %d", __FUNCTION__, idx, vlan);
        goto _eth_type;
    }

    pktBuf->pull(offset);

    if (dstMac == kBcastMac) {
        QList<Device*> list = bcastList_.values(dk.key());
        // FIXME: We need to clone the pktBuf before passing to each
        // device, otherwise only the first device gets the original
        // packet - all subsequent ones get the modified packet!
        // NOTE: modification may not be in the pkt data buffer but
        // in the HDTE pointers - which is bad as well!
        foreach(Device *device, list)
            device->receivePacket(pktBuf);
        goto _exit;
    }

    // Is it destined for us?
    device = deviceList_.value(dk.key());
    if (!device) {
        qDebug("%s: dstMac %012llx is not us", __FUNCTION__, dstMac);
        goto _exit;
    }

    device->receivePacket(pktBuf);

_exit:
    return;
}

void DeviceManager::transmitPacket(PacketBuffer *pktBuf)
{
    port_->sendEmulationPacket(pktBuf);
}

void DeviceManager::clearDeviceNeighbors()
{
    foreach(Device *device, deviceList_)
        device->clearNeighbors();
}

void DeviceManager::getDeviceNeighbors(
        OstProto::DeviceNeighborList *neighborList)
{
    int count = 0;

    foreach(Device *device, deviceList_) {
        OstEmul::DeviceNeighbors *neighList =
            neighborList->AddExtension(OstEmul::devices);
        neighList->set_device_index(count++);
        device->getNeighbors(neighList);
    }
}

void DeviceManager::resolveDeviceNeighbor(PacketBuffer *pktBuf)
{
    Device *device = originDevice(pktBuf);

    if (device)
        device->resolveNeighbor(pktBuf);
}

quint64 DeviceManager::deviceMacAddress(PacketBuffer *pktBuf)
{
    Device *device = originDevice(pktBuf);

    return device ? device->mac() : 0;
}

quint64 DeviceManager::neighborMacAddress(PacketBuffer *pktBuf)
{
    Device *device = originDevice(pktBuf);

    return device ? device->neighborMac(pktBuf) : 0;
}

// ------------------------------------ //
// Private Methods
// ------------------------------------ //

Device* DeviceManager::originDevice(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int offset = 12; // start parsing after mac addresses
    Device dk(this);
    quint16 ethType;
    quint16 vlan;
    int idx = 0;

    // pktBuf will not have the correct dstMac populated, so use bcastMac
    // and search for device by IP

    dk.setMac(kBcastMac);

_eth_type:
    ethType = qFromBigEndian<quint16>(pktData + offset);
    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    if (ethType == 0x8100) {
        offset += 2;
        vlan = qFromBigEndian<quint16>(pktData + offset);
        dk.setVlan(idx++, vlan);
        offset += 2;
        qDebug("%s: idx: %d vlan %d", __FUNCTION__, idx, vlan);
        goto _eth_type;
    }

    pktBuf->pull(offset);

    foreach(Device *device, bcastList_.values(dk.key())) {
        if (device->isOrigin(pktBuf))
            return device;
    }

    qDebug("couldn't find origin device for packet");
    return NULL;
}

void DeviceManager::enumerateDevices(
    const OstProto::DeviceGroup *deviceGroup,
    Operation oper)
{
    Device dk(this);
    OstEmul::VlanEmulation pbVlan = deviceGroup->GetExtension(OstEmul::vlan);
    OstEmul::Device pbDevice = deviceGroup->GetExtension(OstEmul::device);
    int numTags = pbVlan.stack_size();
    int vlanCount = 1;

    for (int i = 0; i < numTags; i++)
        vlanCount *= pbVlan.stack(i).count();

    // If we have no vlans, we still have the non-vlan-segmented LAN
    if (vlanCount == 0)
        vlanCount = 1;

    for (int i = 0; i < vlanCount; i++) {
        for (int j = 0; j < numTags; j++) {
            OstEmul::VlanEmulation::Vlan vlan = pbVlan.stack(j);
            quint16 vlanAdd = i*vlan.step();

            dk.setVlan(j, vlan.vlan_tag() + vlanAdd);
        }

        for (uint k = 0; k < pbDevice.count(); k++) {
            Device *device;
            quint64 macAdd = k * pbDevice.mac().step();
            quint32 ip4Add = k * pbDevice.ip4().step();

            dk.setMac(pbDevice.mac().address() + macAdd);
            dk.setIp4(pbDevice.ip4().address() + ip4Add,
                    pbDevice.ip4().prefix_length(),
                    pbDevice.ip4().default_gateway());

            // TODO: fill in other pbDevice data

            switch (oper) {
                case kAdd:
                    device = new Device(this);
                    *device = dk;
                    deviceList_.insert(dk.key(), device);

                    dk.setMac(kBcastMac);
                    bcastList_.insert(dk.key(), device);
                    qDebug("enumerate(add): %s", qPrintable(device->config()));
                    break;

                case kDelete:
                    device = deviceList_.take(dk.key());
                    qDebug("enumerate(del): %s", qPrintable(device->config()));
                    delete device;

                    dk.setMac(kBcastMac);
                    bcastList_.take(dk.key()); // device already freed above
                    break;

                default:
                    Q_ASSERT(0); // Unreachable
            }
        } // foreach device
    } // foreach vlan
}
