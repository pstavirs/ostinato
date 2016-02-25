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

#include <QHeaderView>
#include <QHostAddress>

#define uintToMacStr(num) \
    QString("%1").arg(num, 6*2, 16, QChar('0')) \
        .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper()
#define macStrToUInt(str) \
    str.remove(QChar(' ')).toULongLong()

enum { kIpNone, kIp4, kIp6, kIpDual };
static QStringList ipStackItems = QStringList()
    << "None" << "IPv4" << "IPv6" << "Dual";

inline UInt128 UINT128(OstEmul::Ip6Address x)
{
    return UInt128(x.hi(), x.lo());
}

inline QString IP4STR(quint32 ip)
{
    return QHostAddress(ip).toString();
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
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

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
    macAddress->setValidator(new QRegExpValidator(reMac, this));
    macStep->setValidator(new QRegExpValidator(reMac, this));
    ipStack->insertItems(0, ipStackItems);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

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
    macAddress->setText(uintToMacStr(mac.address()));
    macStep->setText(uintToMacStr(mac.step()));

    OstEmul::Ip4Emulation ip4 = devGrp->GetExtension(OstEmul::ip4);
    ip4Address->setText(IP4STR(ip4.address()));
    ip4Step->setText(IP4STR(ip4.step()));
    ip4Gateway->setText(IP4STR(ip4.default_gateway()));

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
    mac->set_address(macStrToUInt(macAddress->text()));
    mac->set_step(macStrToUInt(macStep->text()));

    if (ipStack->currentIndex() == kIp4
            || ipStack->currentIndex() == kIpDual) {
        OstEmul::Ip4Emulation *ip4 = devGrp->MutableExtension(OstEmul::ip4);
        ip4->set_address(QHostAddress(ip4Address->text()).toIPv4Address());
        ip4->set_prefix_length(ip4PrefixLength->value());
        ip4->set_default_gateway(
                QHostAddress(ip4Gateway->text()).toIPv4Address());
        ip4->set_step(QHostAddress(ip4Step->text()).toIPv4Address());

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
