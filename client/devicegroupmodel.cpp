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

#include "devicegroupmodel.h"

#include "port.h"

#include "emulproto.pb.h"
#include "uint128.h"

#include <QHostAddress>

enum {
    kName,
    kVlanCount,
    kDeviceCount, // Across all vlans
    kIp,
    kIp4Address,
    kIp6Address,
    kFieldCount
};

static QStringList columns_ = QStringList()
    << "Name"
    << "Vlans"
    << "Devices"
    << "IP Stack"
    << "IPv4 Address"
    << "IPv6 Address";

DeviceGroupModel::DeviceGroupModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    port_ = NULL;
}

int DeviceGroupModel::rowCount(const QModelIndex &parent) const
{
    if (!port_ || parent.isValid())
        return 0;

    return port_->numDeviceGroups();
}

int DeviceGroupModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return columns_.size();
}

QVariant DeviceGroupModel::headerData(
        int section,
        Qt::Orientation orientation,
        int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (orientation) {
        case Qt::Horizontal:
            return columns_[section];
        case Qt::Vertical:
            return QString("%1").arg(section + 1);
        default:
            Q_ASSERT(false); // Unreachable
    }

    return QVariant();
}

QVariant DeviceGroupModel::data(const QModelIndex &index, int role) const
{
    if (!port_ || !index.isValid())
        return QVariant();

    int dgIdx = index.row();
    int field = index.column();

    Q_ASSERT(dgIdx < port_->numDeviceGroups());
    Q_ASSERT(field < kFieldCount);

    const OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(dgIdx);

    Q_ASSERT(devGrp);

    switch (field) {
        case kName:
            switch (role) {
                case Qt::DisplayRole:
                    return QString::fromStdString(devGrp->core().name());
                default:
                    break;
            }
            return QVariant();

        case kVlanCount:
            switch (role) {
                case Qt::DisplayRole:
                    if (int v = vlanCount(devGrp))
                        return v;
                    return QString("None");
                case Qt::TextAlignmentRole:
                    return static_cast<int>(Qt::AlignRight|Qt::AlignVCenter);
                default:
                    break;
            }
            return QVariant();

        case kDeviceCount:
            switch (role) {
                case Qt::DisplayRole:
                    return qMax(vlanCount(devGrp), 1)*devGrp->device_count();
                case Qt::TextAlignmentRole:
                    return static_cast<int>(Qt::AlignRight|Qt::AlignVCenter);
                default:
                    break;
            }
            return QVariant();

        case kIp:
            switch (role) {
                case Qt::DisplayRole:
                    if (devGrp->HasExtension(OstEmul::ip4))
                        if (devGrp->HasExtension(OstEmul::ip6))
                            return QString("Dual Stack");
                        else
                            return QString("IPv4");
                    else if (devGrp->HasExtension(OstEmul::ip6))
                        return QString("IPv6");
                    else
                        return QString("None");
                default:
                    break;
            }
            return QVariant();

        case kIp4Address:
            switch (role) {
                case Qt::DisplayRole:
                    if (devGrp->HasExtension(OstEmul::ip4))
                        return QHostAddress(
                                    devGrp->GetExtension(OstEmul::ip4)
                                                    .address()).toString();
                    else
                        return QString("--");
                default:
                    break;
            }
            return QVariant();

        case kIp6Address:
            switch (role) {
                case Qt::DisplayRole:
                    if (devGrp->HasExtension(OstEmul::ip6)) {
                        OstEmul::Ip6Address ip = devGrp->GetExtension(
                                                    OstEmul::ip6).address();
                        return QHostAddress(
                                    UInt128(ip.hi(), ip.lo()).toArray())
                                        .toString();
                    }
                    else
                        return QString("--");
                default:
                    break;
            }
            return QVariant();

        default:
            Q_ASSERT(false); // unreachable!
            break;
    }

    qWarning("%s: Unsupported field #%d", __FUNCTION__, field);
    return QVariant();
}

bool DeviceGroupModel::setData(
        const QModelIndex & /*index*/,
        const QVariant & /*value*/,
        int /*role*/)
{
    if (!port_)
        return false;

    // TODO; when implementing also implement flags() to
    // return ItemIsEditable
    return false;
}

bool DeviceGroupModel::insertRows(
        int row,
        int count,
        const QModelIndex &parent)
{
    int c = 0;

    Q_ASSERT(!parent.isValid());

    beginInsertRows(parent, row, row+count-1);
    for (int i = 0; i < count; i++) {
        if (port_->newDeviceGroupAt(row))
            c++;
    }
    endInsertRows();

    if (c != count) {
        qWarning("failed to insert rows in DeviceGroupModel at row %d; "
                 "requested = %d, actual = %d", row, count, c);
        return false;
    }

    return true;
}

bool DeviceGroupModel::removeRows(
        int row,
        int count,
        const QModelIndex &parent)
{
    int c = 0;

    Q_ASSERT(!parent.isValid());

    beginRemoveRows(parent, row, row+count-1);
    for (int i = 0; i < count; i++) {
        if (port_->deleteDeviceGroupAt(row))
            c++;
    }
    endRemoveRows();

    if (c != count) {
        qWarning("failed to delete rows in DeviceGroupModel at row %d; "
                 "requested = %d, actual = %d", row, count, c);
        return false;
    }

    return true;
}

void DeviceGroupModel::setPort(Port *port)
{
    beginResetModel();
    port_ = port;
    endResetModel();
}

//
// ---------------------- Private Methods -----------------------
//
int DeviceGroupModel::vlanCount(const OstProto::DeviceGroup *deviceGroup) const
{
    if (!deviceGroup->has_encap()
            || !deviceGroup->encap().HasExtension(OstEmul::vlan))
        return 0;

    OstEmul::VlanEmulation vlan = deviceGroup->encap()
                                        .GetExtension(OstEmul::vlan);
    int numTags = vlan.stack_size();
    int count = 1;

    for (int i = 0; i < numTags; i++)
        count *= vlan.stack(i).count();

    return count;
}
