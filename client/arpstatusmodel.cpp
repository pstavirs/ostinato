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

#include "arpstatusmodel.h"

#include "port.h"

#include "emulproto.pb.h"

#include <QHostAddress>

enum {
    kIp4Address,
    kMacAddress,
    kStatus,
    kFieldCount
};

static QStringList columns_ = QStringList()
    << "IPv4 Address"
    << "Mac Address"
    << "Status";

ArpStatusModel::ArpStatusModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    port_ = NULL;
    deviceIndex_ = -1;
    neighbors_ = NULL;
}

int ArpStatusModel::rowCount(const QModelIndex &parent) const
{
    if (!port_ || deviceIndex_ < 0 || parent.isValid())
        return 0;

    return port_->numArp(deviceIndex_);
}

int ArpStatusModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return columns_.size();
}

QVariant ArpStatusModel::headerData(
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

QVariant ArpStatusModel::data(const QModelIndex &index, int role) const
{
    QString str;

    if (!port_ || deviceIndex_ < 0 || !index.isValid())
        return QVariant();

    int arpIdx = index.row();
    int field = index.column();

    Q_ASSERT(arpIdx < port_->numArp(deviceIndex_));
    Q_ASSERT(field < kFieldCount);

    const OstEmul::ArpEntry &arp = neighbors_->arp(arpIdx);

    switch (field) {
        case kIp4Address:
            switch (role) {
                case Qt::DisplayRole:
                    return QHostAddress(arp.ip4()).toString();
                default:
                    break;
            }
            return QVariant();

        case kMacAddress:
            switch (role) {
                case Qt::DisplayRole:
                    return QString("%1").arg(arp.mac(), 6*2, 16, QChar('0'))
                            .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:")
                            .toUpper();
                default:
                    break;
            }
            return QVariant();

        case kStatus:
            switch (role) {
                case Qt::DisplayRole:
                    return arp.mac() ?
                        QString("Resolved") : QString("Failed");
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

void ArpStatusModel::setDeviceIndex(Port *port, int deviceIndex)
{
    beginResetModel();
    port_ = port;
    deviceIndex_ = deviceIndex;
    if (port_)
        neighbors_ = port_->deviceNeighbors(deviceIndex);
    endResetModel();
}

void ArpStatusModel::updateArpStatus()
{
    // FIXME: why needed?
    beginResetModel();
    endResetModel();
}
