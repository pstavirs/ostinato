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
#include "spinboxdelegate.h"

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

    // Populate the Vlan Table with placeholders - we do this so that
    // user entered values are retained during the lifetime of the dialog
    // even if user is playing around with number of vlan tags
    vlans->setRowCount(kMaxVlanTags);
    vlans->setColumnCount(kVlanColumns);
    vlans->setHorizontalHeaderLabels(vlanTableColumnHeaders);
    for (int i = 0; i < kMaxVlanTags; i++) {
        // Use same default values as defined in .proto
        vlans->setItem(i, kVlanId,
                new QTableWidgetItem(QString::number(100*(i+1))));
        vlans->setItem(i, kVlanCount,
                new QTableWidgetItem(QString::number(1)));
        vlans->setItem(i, kVlanStep,
                new QTableWidgetItem(QString::number(1)));
        vlans->setItem(i, kVlanCfi,
                new QTableWidgetItem(QString::number(0)));
        vlans->setItem(i, kVlanPrio,
                new QTableWidgetItem(QString::number(0)));
        vlans->setItem(i, kVlanTpid,
                new QTableWidgetItem(QString("0x8100")));
    }

    // Set SpinBoxDelegate for all columns except TPID
    SpinBoxDelegate *spd = new SpinBoxDelegate(this);
    spd->setColumnRange(kVlanId, 0, 4095);
    spd->setColumnRange(kVlanStep, 0, 4095);
    spd->setColumnRange(kVlanCfi, 0, 1);
    spd->setColumnRange(kVlanPrio, 0, 7);
    for (int i = 0; i < kVlanColumns; i++) {
        if (i != kVlanTpid)
            vlans->setItemDelegateForColumn(i, spd);
    }

    vlans->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    vlans->resizeRowsToContents();

    // Set vlan tag count *after* adding all items, so connected slots
    // can access the items
    vlanTagCount->setValue(kMaxVlanTags);

    devicePerVlanCount->setRange(1, 0x7fffffff);

    ipStack->insertItems(0, ipStackItems);

    // TODO: DeviceGroup Traversal; hide buttons for now
    // NOTE for implementation: Use a QHash<int, deviceGroup*>
    // to store modified values while traversing; in accept()
    // update port->deviceGroups[] from the QHash
    prev->setHidden(true);
    next->setHidden(true);

    // TODO: Preview devices expanded from deviceGroup configuration
    // for user convenience

    // setup dialog to auto-resize as widgets are hidden or shown
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
    const OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(index_);
    int tagCount = 0;
    // use 1-indexed id so that it matches the port id used in the
    // RFC 4814 compliant mac addresses assigned by default to deviceGroups
    // XXX: use deviceGroupId also as part of the id?
    quint32 id = (port_->id()+1) & 0xff;

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
            vlans->item(i, kVlanTpid)->setText(QString("0x%1")
                                                    .arg(v.tpid(), 0, 16));
        }
    }
    vlanTagCount->setValue(tagCount);

    updateTotalVlanCount();
    devicePerVlanCount->setValue(devGrp->device_count());

    OstEmul::MacEmulation mac = devGrp->GetExtension(OstEmul::mac);
    Q_ASSERT(mac.has_address());
    macAddress->setValue(mac.address());
    macStep->setValue(mac.step());

    OstEmul::Ip4Emulation ip4 = devGrp->GetExtension(OstEmul::ip4);
    // If address is not set, assign one from RFC 2544 space - 192.18.0.0/15
    // Use port Id as the 3rd octet of the address
    ip4Address->setValue(ip4.has_address() ?
            ip4.address() : 0xc6120002 | (id << 8));
    ip4PrefixLength->setValue(ip4.prefix_length());
    ip4Step->setValue(ip4.has_step()? ip4.step() : 1);
    ip4Gateway->setValue(ip4.has_default_gateway() ?
            ip4.default_gateway() : 0xc6120001 | (id << 8));

    OstEmul::Ip6Emulation ip6 = devGrp->GetExtension(OstEmul::ip6);
    // If address is not set, assign one from  RFC 5180 space 2001:0200::/64
    // Use port Id as the 3rd hextet of the address
    ip6Address->setValue(ip6.has_address() ?
            UINT128(ip6.address()) :
            UInt128((0x200102000000ULL | id) << 16, 2));
    ip6PrefixLength->setValue(ip6.prefix_length());
    ip6Step->setValue(ip6.has_step() ? UINT128(ip6.step()) : UInt128(0, 1));
    ip6Gateway->setValue(ip6.has_default_gateway() ?
            UINT128(ip6.default_gateway()) :
            UInt128((0x200102000000ULL | id) << 16, 1));

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
    OstProto::DeviceGroup *devGrp = port_->mutableDeviceGroupByIndex(index_);
    int tagCount = vlanTagCount->value();

    Q_ASSERT(devGrp);

    devGrp->mutable_core()->set_name(name->text().toStdString());

    OstEmul::VlanEmulation *vlan = devGrp->mutable_encap()
                                        ->MutableExtension(OstEmul::vlan);
    vlan->clear_stack();
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

    if (!tagCount)
        devGrp->clear_encap();

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
