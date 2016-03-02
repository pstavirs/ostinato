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

enum { kVlanId, kVlanCount, kVlanStep, kVlanCfi, kVlanPrio, kVlanTpid,
    kVlanColumns };
static QStringList vlanTableColumnHeaders = QStringList()
    << "Vlan Id" << "Count" << "Step" << "CFI/DE" << "Prio" << "TPID";

enum { kIpNone, kIp4, kIp6, kIpDual };
static QStringList ipStackItems = QStringList()
    << "None" << "IPv4" << "IPv6" << "Dual";

inline UInt128 UINT128(OstEmul::Ip6Address x)
{
    return UInt128(x.hi(), x.lo());
}

inline OstEmul::Ip6Address IP6ADDR(UInt128 x)
{
    OstEmul::Ip6Address ip;

    ip.set_hi(x.hi64());
    ip.set_lo(x.lo64());
    return ip;
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
    // Populate the Vlan Table with placeholders - we do this so that
    // user entered values are retained during the lifetime of the dialog
    // even if user is playing around with number of vlan tags
    // TODO: use spinbox delegate with rangecheck for validation
    vlans->setRowCount(kMaxVlanTags);
    vlans->setColumnCount(kVlanColumns);
    vlans->setHorizontalHeaderLabels(vlanTableColumnHeaders);
    for (int i = 0; i < kMaxVlanTags; i++) {
        vlans->setItem(i, kVlanId,
                new QTableWidgetItem(QString::number(100*(i+1))));
        vlans->setItem(i, kVlanCount,
                new QTableWidgetItem(QString::number(10)));
        vlans->setItem(i, kVlanStep,
                new QTableWidgetItem(QString::number(1)));
        vlans->setItem(i, kVlanCfi,
                new QTableWidgetItem(QString::number(0)));
        vlans->setItem(i, kVlanPrio,
                new QTableWidgetItem(QString::number(0)));
        vlans->setItem(i, kVlanTpid,
                new QTableWidgetItem(QString::number(0x8100, 16)));
    }
    // Set vlan tag count *after* adding items so connected slots
    // can access the items
    vlanTagCount->setValue(kMaxVlanTags);

    ipStack->insertItems(0, ipStackItems);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(devicePerVlanCount, SIGNAL(valueChanged(const QString&)),
            this, SLOT(updateTotalDeviceCount()));

    connect(ip4Address, SIGNAL(textEdited(const QString&)),
            this, SLOT(updateIp4Gateway()));
    connect(ip4PrefixLength, SIGNAL(valueChanged(const QString&)),
            this, SLOT(updateIp4Gateway()));
    connect(ip6Address, SIGNAL(textEdited(const QString&)),
            this, SLOT(updateIp6Gateway()));
    connect(ip6PrefixLength, SIGNAL(valueChanged(const QString&)),
            this, SLOT(updateIp6Gateway()));

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

    for (int row = 0; row < kMaxVlanTags; row++)
        vlans->setRowHidden(row, row >= value);

    vlans->setVisible(value > 0);
    updateTotalVlanCount();
}

void DeviceGroupDialog::on_vlans_cellChanged(int row, int col)
{
    if (col != kVlanCount)
        return;

    if (vlans->isRowHidden(row))
        return;

    updateTotalVlanCount();
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

void DeviceGroupDialog::updateTotalVlanCount()
{
    int count = vlanTagCount->value() ? 1 : 0;
    for (int i = 0; i < vlanTagCount->value(); i++)
        count *= vlans->item(i, kVlanCount)->text().toUInt();
    vlanCount->setValue(count);

    updateTotalDeviceCount();
}

void DeviceGroupDialog::updateTotalDeviceCount()
{
    totalDeviceCount->setValue(qMax(vlanCount->value(), 1)
                                    * devicePerVlanCount->value());
}

void DeviceGroupDialog::updateIp4Gateway()
{
    quint32 net = ip4Address->value() & (~0 << (32 - ip4PrefixLength->value()));
    ip4Gateway->setValue(net | 0x01);
}

void DeviceGroupDialog::updateIp6Gateway()
{
    UInt128 net = ip6Address->value()
                        & (~UInt128(0, 0) << (128 - ip6PrefixLength->value()));
    ip6Gateway->setValue(net | UInt128(0, 1));
}

void DeviceGroupDialog::loadDeviceGroup()
{
    OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(index_);
    int tagCount = 0;

    Q_ASSERT(devGrp);

    name->setText(QString::fromStdString(devGrp->core().name()));

    if (devGrp->has_encap() && devGrp->encap().HasExtension(OstEmul::vlan)) {
        OstEmul::VlanEmulation vlan = devGrp->encap()
                                            .GetExtension(OstEmul::vlan);
        tagCount = vlan.stack_size();
        for (int i = 0; i < tagCount; i++) {
            OstEmul::VlanEmulation::Vlan v = vlan.stack(i);
            vlans->item(i, kVlanPrio)->setText(
                    QString::number((v.vlan_tag() >> 13) & 0x7));
            vlans->item(i, kVlanCfi)->setText(
                    QString::number((v.vlan_tag() >> 12) & 0x1));
            vlans->item(i, kVlanId)->setText(
                    QString::number(v.vlan_tag() & 0x0fff));
            vlans->item(i, kVlanCount)->setText(QString::number(v.count()));
            vlans->item(i, kVlanStep)->setText(QString::number(v.step()));
            vlans->item(i, kVlanTpid)->setText(QString::number(v.tpid(), 16));
        }
    }
    vlanTagCount->setValue(tagCount);

    updateTotalVlanCount();
    devicePerVlanCount->setValue(devGrp->device_count());

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
    // ip6 fields don't have default values defined in the .proto
    // because protobuf doesn't allow different default values for
    // embedded message fields, so assign them here
    // Use address 2001:0200::/64 from the RFC 5180 range
    ip6Address->setValue(ip6.has_address() ?
            UINT128(ip6.address()) : UInt128(0x20010200ULL << 32, 2));
    ip6PrefixLength->setValue(ip6.prefix_length());
    ip6Step->setValue(ip6.has_step() ? UINT128(ip6.step()) : UInt128(0, 1));
    ip6Gateway->setValue(ip6.has_default_gateway() ?
            UINT128(ip6.default_gateway()) : UInt128(0x20010200ULL << 32, 1));

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

    OstEmul::VlanEmulation *vlan = devGrp->mutable_encap()
                                        ->MutableExtension(OstEmul::vlan);
    vlan->clear_stack();
    tagCount = vlanTagCount->value();
    for (int i = 0; i < tagCount; i++) {
        OstEmul::VlanEmulation::Vlan *v = vlan->add_stack();
        v->set_vlan_tag(
                  vlans->item(i, kVlanPrio)->text().toUInt() << 13
                | vlans->item(i, kVlanCfi)->text().toUInt() << 12
                | vlans->item(i, kVlanId)->text().toUInt());
        v->set_count(vlans->item(i, kVlanCount)->text().toUInt());
        v->set_step(vlans->item(i, kVlanStep)->text().toUInt());
        v->set_tpid(vlans->item(i, kVlanTpid)->text().toUInt(NULL, 16));
    }

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
        ip6->mutable_address()->CopyFrom(IP6ADDR(ip6Address->value()));
        ip6->set_prefix_length(ip6PrefixLength->value());
        ip6->mutable_step()->CopyFrom(IP6ADDR(ip6Step->value()));
        ip6->mutable_default_gateway()->CopyFrom(IP6ADDR(ip6Gateway->value()));

        if (ipStack->currentIndex() == kIp6)
            devGrp->ClearExtension(OstEmul::ip4);
    }

    if (ipStack->currentIndex() == kIpNone) {
        devGrp->ClearExtension(OstEmul::ip4);
        devGrp->ClearExtension(OstEmul::ip6);
    }
}
