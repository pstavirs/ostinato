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
#include "emuldevice.h"
#include "../common/emulation.h"
#include "hostdevice.h"
#include "interfaceinfo.h"
#include "nulldevice.h"
#include "packetbuffer.h"

#include "../common/emulproto.pb.h"

#include <qendian.h>

const quint64 kBcastMac = 0xffffffffffffULL;

inline UInt128 UINT128(OstEmul::Ip6Address x)
{
    return UInt128(x.hi(), x.lo());
}

inline bool isMacMcast(quint64 mac)
{
    return ((mac >> 40) & 0x01) == 0x01;
}


// XXX: Port owning DeviceManager already uses locks, so we don't use any
// locks within DeviceManager to protect deviceGroupList_ et.al.

DeviceManager::DeviceManager(AbstractPort *parent)
{
    port_ = parent;
}

void DeviceManager::createHostDevices(void)
{
    const InterfaceInfo *ifInfo = port_->interfaceInfo();

    if (!ifInfo)
        return;

    Device *device = HostDevice::create(port_->name(), this);
    device->setMac(ifInfo->mac);

    int count = ifInfo->ip4.size();
    for (int i = 0; i < count; i++) {
        // XXX: Since we can't support multiple IPs with same mac,
        // skip link-local IP - unless it is the only one
        if (((ifInfo->ip4.at(i).address & 0xffff0000) == 0xa9fe0000)
                && (count > 1))
            continue;
        device->setIp4(ifInfo->ip4.at(i).address,
                       ifInfo->ip4.at(i).prefixLength,
                       ifInfo->ip4.at(i).gateway);

        break; // TODO: support multiple IPs with same mac
    }

    count = ifInfo->ip6.size();
    for (int i = 0; i < count; i++) {
        // XXX: Since we can't support multiple IPs with same mac,
        // skip link-local IP - unless it is the only one
        if (((ifInfo->ip6.at(i).address.hi64() >> 48) == 0xfe80)
                && (count > 1))
            continue;
        device->setIp6(ifInfo->ip6.at(i).address,
                       ifInfo->ip6.at(i).prefixLength,
                       ifInfo->ip6.at(i).gateway);

        break; // TODO: support multiple IPs with same mac
    }

    if (deviceList_.contains(device->key())) {
        qWarning("%s: error adding host device %s (EEXIST)",
                __FUNCTION__, qPrintable(device->config()));
        delete device;
        return;
    }
    hostDeviceList_.append(device);
    insertDevice(device->key(), device);
    qDebug("host(add): %s", qPrintable(device->config()));
}

DeviceManager::~DeviceManager()
{
    // Delete *all* devices - host and enumerated
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

    deviceGroup = newDeviceGroup(port_->id());
    deviceGroup->mutable_device_group_id()->set_id(deviceGroupId);
    deviceGroupList_.insert(deviceGroupId, deviceGroup);

    enumerateDevices(deviceGroup, kAdd);

    // Start emulation when first device group is added
    // NOTE: Host devices don't have a deviceGroup and don't need emulation
    if ((deviceGroupCount() == 1) && port_)
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

    // Stop emulation if no device groups remain
    // NOTE: Host devices don't have a deviceGroup and don't need emulation
    if ((deviceGroupCount() == 0) && port_)
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
    // If mac step is 0, silently override to 1 - otherwise we won't have
    // unique DeviceKeys
    if (myDeviceGroup->GetExtension(OstEmul::mac).step() == 0)
        myDeviceGroup->MutableExtension(OstEmul::mac)->set_step(1);
    // Default value for ip6 step should be 1 (not 0)
    if (myDeviceGroup->HasExtension(OstEmul::ip6)
           && !myDeviceGroup->GetExtension(OstEmul::ip6).has_step())
        myDeviceGroup->MutableExtension(OstEmul::ip6)
            ->mutable_step()->set_lo(1);

    enumerateDevices(myDeviceGroup, kAdd);

    return true;
}

int DeviceManager::deviceCount()
{
    return deviceList_.size();
}

void DeviceManager::getDeviceList(
        OstProto::PortDeviceList *deviceList)
{
    foreach(Device *device, sortedDeviceList_) {
        OstEmul::Device *dev =
            deviceList->AddExtension(OstEmul::device);
        device->getConfig(dev);
    }
}

void DeviceManager::receivePacket(PacketBuffer *pktBuf)
{
    uchar *pktData = pktBuf->data();
    int offset = 0;
    EmulDevice dk(this);
    Device *device;
    quint64 dstMac;
    quint16 ethType;
    quint16 vlan;
    int idx = 0;

    // We assume pkt is ethernet
    // TODO: extend for other link layer types

    // All frames we are interested in should be at least 32 bytes
    if (pktBuf->length() < 32) {
        qWarning("short frame of %d bytes, skipping ...", pktBuf->length());
        goto _exit;
    }

    // Extract dstMac
    dstMac = qFromBigEndian<quint32>(pktData + offset);
    offset += 4;
    dstMac = (dstMac << 16) | qFromBigEndian<quint16>(pktData + offset);

    qDebug("dstMac %012llx", dstMac);

    // XXX: Treat multicast as bcast
    if (isMacMcast(dstMac))
        dstMac = kBcastMac;

    dk.setMac(dstMac);
    offset += 2;

    // Skip srcMac - don't care
    offset += 6;

_eth_type:
    // Extract EthType
    ethType = qFromBigEndian<quint16>(pktData + offset);
    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    if (tpidList_.contains(ethType)) {
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
    delete pktBuf;
}

void DeviceManager::transmitPacket(PacketBuffer *pktBuf)
{
    port_->sendEmulationPacket(pktBuf);
}

void DeviceManager::resolveDeviceGateways()
{
    foreach(Device *device, deviceList_) {
        device->resolveGateway();
    }
}

void DeviceManager::clearDeviceNeighbors(Device::NeighborSet set)
{
    foreach(Device *device, deviceList_)
        device->clearNeighbors(set);
}

void DeviceManager::getDeviceNeighbors(
        OstProto::PortNeighborList *neighborList)
{
    int count = 0;

    foreach(Device *device, sortedDeviceList_) {
        OstEmul::DeviceNeighborList *neighList =
            neighborList->AddExtension(OstEmul::device_neighbor);
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
    EmulDevice dk(this);
    quint16 ethType;
    quint16 vlan;
    int idx = 0;

    // Do we have any devices at all?
    if (!deviceCount())
       return NULL;

    // pktBuf will not have the correct dstMac populated, so use bcastMac
    // and search for device by IP

    dk.setMac(kBcastMac);

_eth_type:
    ethType = qFromBigEndian<quint16>(pktData + offset);
    qDebug("%s: ethType 0x%x", __PRETTY_FUNCTION__, ethType);

    if (tpidList_.contains(ethType)) {
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
    EmulDevice dk(this);
    OstEmul::VlanEmulation pbVlan = deviceGroup->encap()
                                        .GetExtension(OstEmul::vlan);
    int numTags = pbVlan.stack_size();
    int n = 1;
    QList<int> vlanCount;

    bool hasIp4 = deviceGroup->HasExtension(OstEmul::ip4);
    bool hasIp6 = deviceGroup->HasExtension(OstEmul::ip6);
    OstEmul::MacEmulation mac = deviceGroup->GetExtension(OstEmul::mac);
    OstEmul::Ip4Emulation ip4 = deviceGroup->GetExtension(OstEmul::ip4);
    OstEmul::Ip6Emulation ip6 = deviceGroup->GetExtension(OstEmul::ip6);

    /*
     * vlanCount[] stores the number of unique vlans at each tag level
     * e.g. for a 3-tag config with 2, 3, 4 vlans at each level respectively
     * vlanCount = [24, 12, 4]
     *      0 - 0, 0, 0
     *      1 - 0, 0, 1
     *      2 - 0, 0, 2
     *      3 - 0, 0, 3
     *      4 - 0, 1, 0
     *      5 - 0, 1, 1
     *      6 - 0, 1, 2
     *      7 - 0, 1, 3
     *      8 - 0, 2, 0
     *      9 - 0, 2, 1
     *     10 - 0, 2, 2
     *     11 - 0, 2, 3
     *     12 - 1, 0, 0
     *     13 - 1, 0, 1
     *     14 - 1, 0, 2
     *     15 - 1, 0, 3
     *     16 - 1, 1, 0
     *     17 - 1, 1, 1
     *     18 - 1, 1, 2
     *     19 - 1, 1, 3
     *     21 - 1, 2, 0
     *     21 - 1, 2, 1
     *     22 - 1, 2, 2
     *     23 - 1, 2, 3
     *
     * Note that vlanCount[0] repesents total-number-of-vlans
     *
     * Another way to think about this is that at a particular vlan tag
     * level, we need to repeat a particular vlan-id as many times as the
     * next level's count before we can increment the vlan-id at that level
     *
     * We use this list to calculate the vlan ids for each tag level for
     * all the vlans.
     *
     * For implementation convenience we append a '1' as the last element
     */
    vlanCount.append(n);
    for (int i = numTags - 1; i >= 0 ; i--) {
        OstEmul::VlanEmulation::Vlan vlan = pbVlan.stack(i);
        n *= vlan.count();
        vlanCount.prepend(n);

        // Update TPID list
        switch (oper) {
            case kAdd:
                tpidList_[vlan.tpid()]++;
                break;
            case kDelete:
                tpidList_[vlan.tpid()]--;
                if (tpidList_[vlan.tpid()] == 0)
                    tpidList_.remove(vlan.tpid());
                break;
            default:
                Q_ASSERT(0); // Unreachable
        }
    }

    QHash<quint16, uint>::const_iterator iter = tpidList_.constBegin();
    qDebug("Port %s TPID List:", port_->name());
    while (iter != tpidList_.constEnd()) {
        qDebug("tpid: %x (%d)", iter.key(), iter.value());
        iter++;
    }

    for (int i = 0; i < vlanCount.at(0); i++) {
        for (int j = 0; j < numTags; j++) {
            OstEmul::VlanEmulation::Vlan vlan = pbVlan.stack(j);
            quint16 vlanAdd = (i/vlanCount.at(j+1) % vlan.count())*vlan.step();

            dk.setVlan(j, vlan.vlan_tag() + vlanAdd, vlan.tpid());
        }

        for (uint k = 0; k < deviceGroup->device_count(); k++) {
            Device *device;
            quint64 macAdd = k * mac.step();
            quint32 ip4Add = k * ip4.step();
            UInt128 ip6Add = UINT128(ip6.step()) * k;

            dk.setMac(mac.address() + macAdd);
            if (hasIp4)
                dk.setIp4(ip4.address() + ip4Add,
                          ip4.prefix_length(),
                          ip4.default_gateway());
            if (hasIp6)
                dk.setIp6(UINT128(ip6.address()) + ip6Add,
                          ip6.prefix_length(),
                          UINT128(ip6.default_gateway()));

            switch (oper) {
                case kAdd:
                    if (deviceList_.contains(dk.key())) {
                        qWarning("%s: error adding device %s (EEXIST)",
                                __FUNCTION__, qPrintable(dk.config()));
                        break;
                    }
                    device = new EmulDevice(this);
                    *device = dk;
                    insertDevice(dk.key(), device);
                    qDebug("enumerate(add): %p %s", device, qPrintable(device->config()));
                    break;

                case kDelete:
                    device = deviceList_.value(dk.key());
                    if (!device) {
                        qWarning("%s: error deleting device %s (NOTFOUND)",
                                __FUNCTION__, qPrintable(dk.config()));
                        break;
                    }
                    qDebug("enumerate(del): %p %s", device, qPrintable(device->config()));
                    deleteDevice(dk.key());
                    break;

                default:
                    Q_ASSERT(0); // Unreachable
            }
        } // foreach device
    } // foreach vlan
}

bool DeviceManager::insertDevice(DeviceKey key, Device *device)
{
    deviceList_.insert(key, device);
    sortedDeviceList_.insert(key, device);

    NullDevice bcastDevice = *(static_cast<NullDevice*>(device));
    bcastDevice.setMac(kBcastMac);

    bcastList_.insert(bcastDevice.key(), device);

    return true;
}

bool DeviceManager::deleteDevice(DeviceKey key)
{
    Device *device = deviceList_.take(key);
    if (!device)
        return false;
    sortedDeviceList_.take(key);

    NullDevice bcastDevice = *(static_cast<NullDevice*>(device));
    bcastDevice.setMac(kBcastMac);
    bcastList_.take(bcastDevice.key());

    delete device;
    return true;
}

