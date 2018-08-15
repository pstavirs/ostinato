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

#include "devicemodel.h"

#include "arpstatusmodel.h"
#include "ndpstatusmodel.h"
#include "port.h"

#include "emulproto.pb.h"
#include "uint128.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHostAddress>

enum {
    kMacAddress,
    kVlans,
    kIp4Address,
    kIp4Gateway,
    kIp6Address,
    kIp6Gateway,
    kArpInfo,
    kNdpInfo,
    kFieldCount
};

static QStringList columns_ = QStringList()
    << "Mac"
    << "Vlans"
    << "IPv4 Address"
    << "IPv4 Gateway"
    << "IPv6 Address"
    << "IPv6 Gateway"
    << "ARP"
    << "NDP";

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    port_ = NULL;
    arpStatusModel_ = new ArpStatusModel(this);
    ndpStatusModel_ = new NdpStatusModel(this);
}

int DeviceModel::rowCount(const QModelIndex &parent) const
{
    if (!port_ || parent.isValid())
        return 0;

    return port_->numDevices();
}

int DeviceModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return columns_.size();
}

QVariant DeviceModel::headerData(
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

QVariant DeviceModel::data(const QModelIndex &index, int role) const
{
    QString str;

    if (!port_ || !index.isValid())
        return QVariant();

    int devIdx = index.row();
    int field = index.column();

    Q_ASSERT(devIdx < port_->numDevices());
    Q_ASSERT(field < kFieldCount);

    const OstEmul::Device *dev = port_->deviceByIndex(devIdx);

    Q_ASSERT(dev);

    switch (field) {
        case kMacAddress:
            switch (role) {
                case Qt::DisplayRole:
                    return QString("%1").arg(dev->mac(), 6*2, 16, QChar('0'))
                            .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:")
                            .toUpper();
                default:
                    break;
            }
            return QVariant();

        case kVlans:
            switch (role) {
                case Qt::DisplayRole:
                    if (!dev->vlan_size())
                        return QString("None");
                    for (int i = 0; i < dev->vlan_size(); i++)
                        str.append(i == 0 ? "" : ", ")
                           .append(QString::number(dev->vlan(i) & 0xfff));
                    return str;
                default:
                    break;
            }
            return QVariant();

        case kIp4Address:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip4_prefix_length())
                        return QString("%1/%2")
                            .arg(QHostAddress(dev->ip4()).toString())
                            .arg(dev->ip4_prefix_length());
                    else
                        return QString("--");
                default:
                    break;
            }
            return QVariant();

        case kIp4Gateway:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip4_prefix_length())
                        return QHostAddress(dev->ip4_default_gateway())
                                    .toString();
                    else
                        return QString("--");
                default:
                    break;
            }
            return QVariant();

        case kIp6Address:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip6_prefix_length()) {
                        OstEmul::Ip6Address ip = dev->ip6();
                        return QString("%1/%2")
                            .arg(QHostAddress(UInt128(ip.hi(), ip.lo())
                                        .toArray()).toString())
                            .arg(dev->ip6_prefix_length());
                    }
                    else
                        return QString("--");
                default:
                    break;
            }
            return QVariant();

        case kIp6Gateway:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip6_prefix_length()) {
                        OstEmul::Ip6Address ip = dev->ip6_default_gateway();
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

        case kArpInfo:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip4_prefix_length())
                        return QString("%1/%2")
                                .arg(port_->numArpResolved(devIdx))
                                .arg(port_->numArp(devIdx));
                    else
                        return QString("--");
                default:
                    if (dev->has_ip4_prefix_length())
                        return drillableStyle(role);
                    break;
            }
            return QVariant();

        case kNdpInfo:
            switch (role) {
                case Qt::DisplayRole:
                    if (dev->has_ip6_prefix_length())
                        return QString("%1/%2")
                                .arg(port_->numNdpResolved(devIdx))
                                .arg(port_->numNdp(devIdx));
                    else
                        return QString("--");
                default:
                    if (dev->has_ip6_prefix_length())
                        return drillableStyle(role);
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

void DeviceModel::setPort(Port *port)
{
    beginResetModel();
    port_ = port;
    if (port_)
        connect(port_, SIGNAL(deviceInfoChanged()), SLOT(updateDeviceList()));
    endResetModel();
}

QAbstractItemModel* DeviceModel::detailModel(const QModelIndex &index)
{
    if (!index.isValid())
        return NULL;

    switch(index.column()) {
        case kArpInfo:
            arpStatusModel_->setDeviceIndex(port_, index.row());
            return arpStatusModel_;
        case kNdpInfo:
            ndpStatusModel_->setDeviceIndex(port_, index.row());
            return ndpStatusModel_;
        default:
            return NULL;
    }
}

void DeviceModel::updateDeviceList()
{
    beginResetModel();
    endResetModel();
}

// Style roles for drillable fields
QVariant DeviceModel::drillableStyle(int role) const
{
    QFont f;
    switch (role) {
        case Qt::ToolTipRole:
            return QString("Click for details ...");
        case Qt::ForegroundRole:
            return QBrush(QColor(Qt::blue));
        case Qt::FontRole:
            f.setUnderline(true);
            return f;
        default:
            break;
    }
    return QVariant();
}

