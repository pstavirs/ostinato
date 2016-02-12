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

enum {
    kName,
    kCount,
    kVlan,
    kIp,
    kFieldCount
};

static QStringList columns_ = QStringList()
    << "Name"
    << "Count"
    << "Vlan"
    << "IP";

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

    OstProto::DeviceGroup *devGrp = port_->deviceGroupByIndex(dgIdx);

    switch (field) {
        case kName:
            switch (role) {
                case Qt::DisplayRole:
                    return QString::fromStdString(devGrp->core().name());
                default:
                    break;
            }
            return QVariant();

        case kCount:
            switch (role) {
                case Qt::DisplayRole:
                    return devGrp->device_count();
                case Qt::TextAlignmentRole:
                    return Qt::AlignRight;
                default:
                    break;
            }
            return QVariant();

        case kVlan:
            switch (role) {
                case Qt::CheckStateRole:
                    if (devGrp->has_encap()
                            && devGrp->encap().HasExtension(OstEmul::vlan)
                            && devGrp->encap().GetExtension(OstEmul::vlan)
                                    .stack_size())
                        return Qt::Checked;
                    return Qt::Unchecked;
                case Qt::TextAlignmentRole:
                    return Qt::AlignCenter;
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

        default:
            break;
    }

    qWarning("%s: Unsupported field #%d", __FUNCTION__, field);
    return QVariant();
}

bool DeviceGroupModel::setData(
        const QModelIndex &index,
        const QVariant &value,
        int role)
{
    if (!port_)
        return false;

    // FIXME
    return false;
}

void DeviceGroupModel::setPort(Port *port)
{
    port_ = port;
    reset();
}
