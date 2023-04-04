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
#include "xqlocale.h"

#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QPalette>

// XXX: Keep the enum in sync with it's string
enum {
    kTxPkts,
    kRxPkts,
    kTxBytes,
    kRxBytes,
    kMaxStreamStats
};
static QStringList statTitles = QStringList()
    << "Tx Pkts"
    << "Rx Pkts"
    << "Tx Bytes"
    << "Rx Bytes";

// XXX: Keep the enum in sync with it's string
enum {
    kAggrTxPkts,
    kAggrRxPkts,
    kAggrPktLoss,
    kTxDuration,
    kAvgTxFrameRate,
    kAvgRxFrameRate,
    kAvgTxBitRate,
    kAvgRxBitRate,
    kAvgLatency,
    kMaxAggrStreamStats
};
static QStringList aggrStatTitles = QStringList()
    << "Total\nTx Pkts"
    << "Total\nRx Pkts"
    << "Total\nPkt Loss"
    << "Duration\n(secs)"
    << "Avg\nTx PktRate"
    << "Avg\nRx PktRate"
    << "Avg\nTx BitRate"
    << "Avg\nRx BitRate"
    << "Avg\nLatency";

static const uint kAggrGuid = 0xffffffff;

StreamStatsModel::StreamStatsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    clearStats();
}

int StreamStatsModel::rowCount(const QModelIndex &/*parent*/) const
{
    return guidList_.size();
}

int StreamStatsModel::columnCount(const QModelIndex &/*parent*/) const
{
    if (!portList_.size())
        return 0;

    return kMaxAggrStreamStats + portList_.size() * kMaxStreamStats;
}

QVariant StreamStatsModel::headerData(
        int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (orientation) {
    case Qt::Horizontal: // Column Header
        if (section < kMaxAggrStreamStats)
            return aggrStatTitles.at(section % kMaxAggrStreamStats);

        section -= kMaxAggrStreamStats;
        return QString("Port %1-%2\n%3")
                        .arg(portList_.at(section/kMaxStreamStats).first)
                        .arg(portList_.at(section/kMaxStreamStats).second)
                        .arg(statTitles.at(section % kMaxStreamStats));
    case Qt::Vertical:   // Row Header
        if (section == (guidList_.size() - 1))
            return QString("GUID Total");
        return QString("Stream GUID %1")
                        .arg(guidList_.at(section));
    default:
        break;
    }
    return QVariant();
}

QVariant StreamStatsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole)
        return Qt::AlignRight;

    int portColumn = index.column() - kMaxAggrStreamStats;

    // Stylesheets typically don't use or set palette colors, so if
    // using one, don't use palette colors
    if ((role == Qt::BackgroundRole) && qApp->styleSheet().isEmpty()) {
        QPalette palette = QApplication::palette();
        if (index.row() == (guidList_.size() - 1)) // Aggregate Row
            return palette.dark();
        if (portColumn < 0)                        // Aggregate Column
            return palette.alternateBase();
        if ((portColumn/kMaxStreamStats) & 1)      // Color alternate Ports
            return palette.alternateBase();
    }

    Guid guid = guidList_.at(index.row());
    if ((role == Qt::ForegroundRole && qApp->styleSheet().isEmpty())) {
        QPalette palette = QApplication::palette();
        if ((index.column() == kAggrPktLoss)
                && aggrGuidStats_.value(guid).pktLoss)
            return palette.link();
        if (index.row() == (guidList_.size() - 1)) // Aggregate Row
            return palette.brightText();
    }

    if (role == Qt::FontRole ) {
        if (index.row() == (guidList_.size() - 1)) { // Aggregate Row
            QFont font;
            font.setBold(true);
            return font;
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    if (index.column() < kMaxAggrStreamStats) {
        int stat = index.column() % kMaxAggrStreamStats;
        switch (stat) {
        case kAggrRxPkts:
            return QString("%L1").arg(aggrGuidStats_.value(guid).rxPkts);
        case kAggrTxPkts:
            return QString("%L1").arg(aggrGuidStats_.value(guid).txPkts);
        case kAggrPktLoss:
            return QString("%L1").arg(aggrGuidStats_.value(guid).pktLoss);
        case kTxDuration:
            return QString("%L1").arg(aggrGuidStats_.value(guid).txDuration);
        case kAvgTxFrameRate:
            return aggrGuidStats_.value(guid).txDuration <= 0 ? QString("-") :
                QString("%L1").arg(
                    aggrGuidStats_.value(guid).txPkts
                        / aggrGuidStats_.value(guid).txDuration);
        case kAvgRxFrameRate:
            return aggrGuidStats_.value(guid).txDuration <= 0 ? QString("-") :
                 QString("%L1").arg(
                    aggrGuidStats_.value(guid).rxPkts
                        / aggrGuidStats_.value(guid).txDuration);
        case kAvgTxBitRate:
            return aggrGuidStats_.value(guid).txDuration <= 0 ? QString("-") :
                 XLocale().toBitRateString(
                    (aggrGuidStats_.value(guid).txBytes
                            + 24 * aggrGuidStats_.value(guid).txPkts) * 8
                        / aggrGuidStats_.value(guid).txDuration);
        case kAvgRxBitRate:
            return aggrGuidStats_.value(guid).txDuration <= 0 ? QString("-") :
                 XLocale().toBitRateString(
                    (aggrGuidStats_.value(guid).rxBytes
                            + 24 * aggrGuidStats_.value(guid).rxPkts) * 8
                        / aggrGuidStats_.value(guid).txDuration);
        case kAvgLatency:
            return aggrGuidStats_.value(guid).latencyCount <= 0
                        || aggrGuidStats_.value(guid).latencySum <= 0 ? QString("-") :
                XLocale().toTimeIntervalString(
                    aggrGuidStats_.value(guid).latencySum
                        / aggrGuidStats_.value(guid).latencyCount);
        default:
            break;
        };
        return QVariant();
    }

    PortGroupPort pgp = portList_.at(portColumn/kMaxStreamStats);
    int stat = portColumn % kMaxStreamStats;

    switch (stat) {
    case kRxPkts:
        return QString("%L1").arg(streamStats_.value(guid).value(pgp).rxPkts);
    case kTxPkts:
        return QString("%L1").arg(streamStats_.value(guid).value(pgp).txPkts);
    case kRxBytes:
        return QString("%L1").arg(streamStats_.value(guid).value(pgp).rxBytes);
    case kTxBytes:
        return QString("%L1").arg(streamStats_.value(guid).value(pgp).txBytes);
    default:
        break;
    }

    return QVariant();
}

Qt::DropActions StreamStatsModel::supportedDropActions() const
{
    return Qt::IgnoreAction; // read-only model, doesn't accept any data
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
    aggrGuidStats_.clear();

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
        StreamStats &aggrPort = streamStats_[kAggrGuid][pgp];
        AggrGuidStats &aggrGuid = aggrGuidStats_[guid];
        AggrGuidStats &aggrAggr = aggrGuidStats_[kAggrGuid];

        ss.rxPkts = s.rx_pkts();
        ss.txPkts = s.tx_pkts();
        ss.rxBytes = s.rx_bytes();
        ss.txBytes = s.tx_bytes();
        ss.rxLatency = s.latency();

        aggrPort.rxPkts += ss.rxPkts;
        aggrPort.txPkts += ss.txPkts;
        aggrPort.rxBytes += ss.rxBytes;
        aggrPort.txBytes += ss.txBytes;

        aggrGuid.rxPkts += ss.rxPkts;
        aggrGuid.txPkts += ss.txPkts;
        aggrGuid.pktLoss += ss.txPkts - ss.rxPkts;
        aggrGuid.rxBytes += ss.rxBytes;
        aggrGuid.txBytes += ss.txBytes;
        if (s.tx_duration() > aggrGuid.txDuration)
            aggrGuid.txDuration = s.tx_duration(); // XXX: use largest or avg?
        if (ss.rxLatency) {
            aggrGuid.latencySum += ss.rxLatency;
            aggrGuid.latencyCount++;
        }

        aggrAggr.rxPkts += ss.rxPkts;
        aggrAggr.txPkts += ss.txPkts;
        aggrAggr.pktLoss += ss.txPkts - ss.rxPkts;
        aggrAggr.rxBytes += ss.rxBytes;
        aggrAggr.txBytes += ss.txBytes;
        if (aggrGuid.txDuration > aggrAggr.txDuration)
            aggrAggr.txDuration = aggrGuid.txDuration;
        if (ss.rxLatency) {
            aggrAggr.latencySum += ss.rxLatency;
            aggrAggr.latencyCount++;
        }

        if (!portList_.contains(pgp))
            portList_.append(pgp);
        if (!guidList_.contains(guid))
            guidList_.append(guid);
    }

    if (guidList_.size() && !guidList_.contains(kAggrGuid))
        guidList_.append(kAggrGuid);

    std::sort(guidList_.begin(), guidList_.end());

#if QT_VERSION >= 0x040600
    endResetModel();
#else
    reset();
#endif

    // Prevent receiving any future updates from this sender
    disconnect(sender(), 0, this, 0);
}
