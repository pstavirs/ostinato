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

#include "streamstatsmodel.h"

#include "protocol.pb.h"

// XXX: Keep the enum in sync with it's string
enum {
    kRxPkts,
    kTxPkts,
    kRxBytes,
    kTxBytes,
    kMaxStreamStats
};
static QStringList statTitles = QStringList()
    << "Rx Pkts"
    << "Tx Pkts"
    << "Rx Bytes"
    << "Tx Bytes";

StreamStatsModel::StreamStatsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int StreamStatsModel::rowCount(const QModelIndex &parent) const
{
    return guidList_.size();
}

int StreamStatsModel::columnCount(const QModelIndex &parent) const
{
    return portList_.size() * kMaxStreamStats;
}

QVariant StreamStatsModel::headerData(
        int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (orientation) {
    case Qt::Horizontal: // Column Header
        return QString("Port %1-%2\n%3")
                        .arg(portList_.at(section/kMaxStreamStats).first)
                        .arg(portList_.at(section/kMaxStreamStats).second)
                        .arg(statTitles.at(section % kMaxStreamStats));
    case Qt::Vertical:   // Row Header
        return QString("Stream GUID %1")
                        .arg(guidList_.at(section));
    default:
        break;
    }
    return QVariant();
}

QVariant StreamStatsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    Guid guid = guidList_.at(index.row());
    PortGroupPort pgp = portList_.at(index.column()/kMaxStreamStats);
    int stat = index.column() % kMaxStreamStats;

    switch (stat) {
    case kRxPkts:
        return streamStats_.value(guid).value(pgp).rxPkts;
    case kTxPkts:
        return streamStats_.value(guid).value(pgp).txPkts;
    case kRxBytes:
        return streamStats_.value(guid).value(pgp).rxBytes;
    case kTxBytes:
        return streamStats_.value(guid).value(pgp).txBytes;
    default:
        break;
    }

    return QVariant();
}

// --------------------------------------------- //
// Slots
// --------------------------------------------- //
void StreamStatsModel::clearStats()
{
#if QT_VERSION >= 0x040600
    beginResetModel();
#endif

    guidList_.clear();
    portList_.clear();
    streamStats_.clear();

#if QT_VERSION >= 0x040600
    endResetModel();
#else
    reset();
#endif
}

void StreamStatsModel::appendStreamStatsList(
        quint32 portGroupId,
        const OstProto::StreamStatsList *stats)
{
    int n = stats->stream_stats_size();

#if QT_VERSION >= 0x040600
    beginResetModel();
#endif

    for (int i = 0; i < n; i++) {
        const OstProto::StreamStats &s = stats->stream_stats(i);
        PortGroupPort pgp = PortGroupPort(portGroupId, s.port_id().id());
        Guid guid = s.stream_guid().id();
        StreamStats &ss = streamStats_[guid][pgp];

        ss.rxPkts = s.rx_pkts();
        ss.txPkts = s.tx_pkts();
        ss.rxBytes = s.rx_bytes();
        ss.txBytes = s.tx_bytes();

        if (!portList_.contains(pgp))
            portList_.append(pgp);
        if (!guidList_.contains(guid))
            guidList_.append(guid);
    }

#if QT_VERSION >= 0x040600
    endResetModel();
#else
    reset();
#endif

    // Prevent receiving any future updates from this sender
    disconnect(sender(), 0, this, 0);
}
