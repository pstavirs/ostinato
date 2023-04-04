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

#ifndef _STREAM_STATS_MODEL_H
#define _STREAM_STATS_MODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>
#include <QPair>
#include <QStringList>

namespace OstProto {
    class StreamStatsList;
}

class StreamStatsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    StreamStatsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    int columnCount(const QModelIndex &parent=QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    Qt::DropActions supportedDropActions() const;

public slots:
    void clearStats();
    void appendStreamStatsList(quint32 portGroupId,
                               const OstProto::StreamStatsList *stats);
private:
    typedef QPair<uint, uint> PortGroupPort; // Pair = (PortGroupId, PortId)
    typedef uint Guid;
    struct StreamStats {
        quint64 rxPkts;
        quint64 txPkts;
        quint64 rxBytes;
        quint64 txBytes;
        quint64 rxLatency;
    };
    struct AggrGuidStats {
        quint64 rxPkts;
        quint64 txPkts;
        quint64 rxBytes;
        quint64 txBytes;
        qint64 pktLoss;
        double txDuration;
        quint64 latencySum;
        uint latencyCount;
    };
    QList<Guid> guidList_;
    QList<PortGroupPort> portList_;
    QHash<Guid, QHash<PortGroupPort, StreamStats> > streamStats_;
    QHash<Guid, AggrGuidStats> aggrGuidStats_;
};
#endif

