/*
Copyright (C) 2016 Srivats P.

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

#include "devicegroupdialog.h"

#include "port.h"

#include "emulproto.pb.h"
#include "uint128.h"

#include "macedit.h"
#include "ip4edit.h"
#include <QHeaderView>
#include <QHostAddress>

enum { kIpNone, kIp4, kIp6, kIpDual };
static QStringList ipStackItems = QStringList()
    << "None" << "IPv4" << "IPv6" << "Dual";

inline UInt128 UINT128(OstEmul::Ip6Address x)
{
    return UInt128(x.hi(), x.lo());
}

inline QString IP6STR(OstEmul::Ip6Address ip)
{
    return QHostAddress(UINT128(ip).toArray()).toString();
}

DeviceGroupDialog::DeviceGroupDialog(
        Port *port,
        int deviceGroupIndex,
        QWidget *parent,
        Qt::WindowFlags flags)
    : QDialog(parent, flags), port_(port), index_(deviceGroupIndex)
{
    // Setup the Dialog
    setupUi(this);
    vlanTagCount->setRange(0, kMaxVlanTags);
#if 0 // FIXME: not working
    qDebug("vlan def size: %d", vlans->verticalHeader()->defaultSectionSize());
    qDebug("vlan min size: %d", vlans->verticalHeader()->minimumSectionSize());
    vlans->verticalHeader()->setDefaultSectionSize(
        vlans->verticalHeader()->minimumSectionSize());
    qDebug("vlan def size: %d", vlans->verticalHeader()->defaultSectionSize());
    qDebug("vlan min size: %d", vlans->verticalHeader()->minimumSectionSize());
#endif
    ipStack->insertItems(0, ipStackItems);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(ip4Address, SIGNAL(textEdited(const QString&)),
            this, SLOT(updateIp4Gateway()));
    connect(ip4PrefixLength, SIGNAL(valueChanged(const QString&)),
            this, SLOT(updateIp4Gateway()));

    loadDeviceGroup();
}

void DeviceGroupDialog::accept()
{
    storeDeviceGroup();
    QDialog::accept();
}

//
// Private Slots
//
void DeviceGroupDialog::on_vlanTagCount_valueChanged(int value)
{
    Q_ASSERT((value >= 0) && (value <= kMaxVlanTags));

    vlans->setVisible(value > 0);
}

void DeviceGroupDialog::on_ipStack_currentIndexChanged(int index)
{
    switch (index) {
        case kIpNone:
            ip4->hide();
            ip6->hide();
            break;
        case kIp4:
            ip4->show();
            ip6->hide();
            break;
        case kIp6:
            ip4->hide();
            ip6->show();
            break;
        case kIpDual:
            ip4->show();
            ip6->show();
            break;
        default:
            Q_ASSERT(false); // Unreachable!
            break;
    }
}

void DeviceGroupDialog::updateIp4Gateway()
{
    quint32 net = ip4Address->value() & (~0 << (32 - ip4PrefixLength->value()));
    ip4Gateway->setValue(net | 0x01);
}

void DeviceGroupDialog::loadDeviceGroup()
{
    OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(index_);
    int tagCount = 0;
    int totalVlans;

    Q_ASSERT(devGrp);

    name->setText(QString::fromStdString(devGrp->core().name()));

    if (devGrp->has_encap() && devGrp->encap().HasExtension(OstEmul::vlan))
        tagCount = devGrp->encap().GetExtension(OstEmul::vlan).stack_size();
    vlanTagCount->setValue(tagCount);

    // FIXME: vlan table widget

    totalVlans = totalVlanCount();
    vlanCount->setText(QString::number(totalVlans));
    devicePerVlanCount->setValue(devGrp->device_count());
    totalDeviceCount->setText(
            QString::number(totalVlans * devGrp->device_count()));

    OstEmul::MacEmulation mac = devGrp->GetExtension(OstEmul::mac);
    if (!mac.has_address()) {
        // Mac address as per RFC 4814 Sec 4.2
        // (RR & 0xFC):PP:PP:RR:RR:RR
        // where RR is a random number, PP:PP is 1-indexed port index
        // NOTE: although qrand() return type is a int, the max value
        // is RAND_MAX (stdlib.h) which is often 16-bit only, so we
        // use two random numbers
        quint32 r1 = qrand(), r2 = qrand();
        quint64 mac;
        mac = quint64(r1 & 0xfc00) << 32
            | quint64(port_->id() + 1) << 24
            | quint64((r1 & 0xff) << 16 | (r2 & 0xffff));
        macAddress->setValue(mac);
    }
    else
        macAddress->setValue(mac.address());
    macStep->setValue(mac.step());

    OstEmul::Ip4Emulation ip4 = devGrp->GetExtension(OstEmul::ip4);
    ip4Address->setValue(ip4.address());
    ip4PrefixLength->setValue(ip4.prefix_length());
    ip4Step->setValue(ip4.step());
    ip4Gateway->setValue(ip4.default_gateway());

    OstEmul::Ip6Emulation ip6 = devGrp->GetExtension(OstEmul::ip6);
    ip6Address->setText(IP6STR(ip6.address()));
    ip6Step->setText(IP6STR(ip6.step()));
    ip6Gateway->setText(IP6STR(ip6.default_gateway()));

    int stk = kIpNone;
    if (devGrp->HasExtension(OstEmul::ip4))
        if (devGrp->HasExtension(OstEmul::ip6))
            stk = kIpDual;
        else
            stk = kIp4;
    else if (devGrp->HasExtension(OstEmul::ip6))
        stk = kIp6;
    ipStack->setCurrentIndex(stk);
}

void DeviceGroupDialog::storeDeviceGroup()
{
    OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(index_);
    int tagCount = 0;

    Q_ASSERT(devGrp);

    devGrp->mutable_core()->set_name(name->text().toStdString());

    tagCount = vlanTagCount->value();
    // FIXME: vlan table widget

    devGrp->set_device_count(devicePerVlanCount->value());

    OstEmul::MacEmulation *mac = devGrp->MutableExtension(OstEmul::mac);
    mac->set_address(macAddress->value());
    mac->set_step(macStep->value());

    if (ipStack->currentIndex() == kIp4
            || ipStack->currentIndex() == kIpDual) {
        OstEmul::Ip4Emulation *ip4 = devGrp->MutableExtension(OstEmul::ip4);
        ip4->set_address(ip4Address->value());
        ip4->set_prefix_length(ip4PrefixLength->value());
        ip4->set_default_gateway(ip4Gateway->value());
        ip4->set_step(ip4Step->value());

        if (ipStack->currentIndex() == kIp4)
            devGrp->ClearExtension(OstEmul::ip6);
    }

    if (ipStack->currentIndex() == kIp6
            || ipStack->currentIndex() == kIpDual) {
        OstEmul::Ip6Emulation *ip6 = devGrp->MutableExtension(OstEmul::ip6);
        Q_IPV6ADDR w;
        UInt128 x;

        w = QHostAddress(ip6Address->text()).toIPv6Address();
        x = UInt128((quint8*)&w);
        ip6->mutable_address()->set_hi(x.hi64());
        ip6->mutable_address()->set_lo(x.lo64());

        ip6->set_prefix_length(ip4PrefixLength->value());

        w = QHostAddress(ip6Gateway->text()).toIPv6Address();
        x = UInt128((quint8*)&w);
        ip6->mutable_default_gateway()->set_hi(x.hi64());
        ip6->mutable_default_gateway()->set_lo(x.lo64());

        w = QHostAddress(ip6Step->text()).toIPv6Address();
        x = UInt128((quint8*)&w);
        ip6->mutable_step()->set_hi(x.hi64());
        ip6->mutable_step()->set_lo(x.lo64());

        if (ipStack->currentIndex() == kIp6)
            devGrp->ClearExtension(OstEmul::ip4);
    }

    if (ipStack->currentIndex() == kIpNone) {
        devGrp->ClearExtension(OstEmul::ip4);
        devGrp->ClearExtension(OstEmul::ip6);
    }
}

int DeviceGroupDialog::totalVlanCount()
{
    // FIXME
    return 1;
}
