/*
Copyright (C) 2010 Srivats P.

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

#ifndef _PORT_STATS_MODEL_H
#define _PORT_STATS_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class QTimer;

typedef enum {
    // State
    e_STATE_START = 0,

    e_LINK_STATE = e_STATE_START,
    e_TRANSMIT_STATE,
    e_CAPTURE_STATE,

    e_STATE_END = e_CAPTURE_STATE,

    // Statistics
    e_STATISTICS_START,

    e_STAT_FRAMES_RCVD = e_STATISTICS_START,
    e_STAT_FRAMES_SENT,
    e_STAT_FRAME_SEND_RATE,
    e_STAT_FRAME_RECV_RATE,
    e_STAT_BYTES_RCVD,
    e_STAT_BYTES_SENT,
    e_STAT_BYTE_SEND_RATE,
    e_STAT_BYTE_RECV_RATE,
#if 0
    e_STAT_FRAMES_RCVD_NIC,
    e_STAT_FRAMES_SENT_NIC,
    e_STAT_BYTES_RCVD_NIC,
    e_STAT_BYTES_SENT_NIC,
#endif

    // Rx Errors 
    e_STAT_RX_DROPS,
    e_STAT_RX_ERRORS,
    e_STAT_RX_FIFO_ERRORS,
    e_STAT_RX_FRAME_ERRORS,

    e_STATISTICS_END = e_STAT_RX_FRAME_ERRORS,

    e_STAT_MAX
} PortStat;

static QStringList PortStatName = (QStringList()
    << "Link State"
    << "Transmit State"
    << "Capture State"

    << "Frames Received"
    << "Frames Sent"
    << "Frame Send Rate (fps)"
    << "Frame Receive Rate (fps)"
    << "Bytes Received"
    << "Bytes Sent"
    << "Byte Send Rate (Bps)"
    << "Byte Receive Rate (Bps)"
#if 0
    << "Frames Received (NIC)"
    << "Frames Sent (NIC)"
    << "Bytes Received (NIC)"
    << "Bytes Sent (NIC)"
#endif
    << "Receive Drops"
    << "Receive Errors"
    << "Receive Fifo Errors"
    << "Receive Frame Errors"
);

static QStringList LinkStateName = (QStringList()
    << "Unknown"
    << "Down"
    << "Up"
);

static QStringList BoolStateName = (QStringList()
    << "Off"
    << "On"
);

class PortGroupList;

class PortStatsModel : public QAbstractTableModel
{
    Q_OBJECT

    public:

        PortStatsModel(PortGroupList *p, QObject *parent = 0);
        ~PortStatsModel();

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, 
            int role = Qt::DisplayRole) const;

        class PortGroupAndPortList {
            public:
            uint portGroupId;
            QList<uint> portList;
        };
        void portListFromIndex(QModelIndexList indices, 
                QList<PortGroupAndPortList> &portList);

    public slots:
        void when_portListChanged();
        void on_portStatsUpdate(int port, void*stats);
        void when_portGroup_stats_update(quint32 portGroupId);

    private slots:
        void updateStats();

    private:
        PortGroupList    *pgl;

        // numPorts stores the num of ports per portgroup
        // in the same order as the portgroups are index in the pgl
        // Also it stores them as cumulative totals
        QList<quint16>    numPorts;

        QTimer *timer;

        void getDomainIndexes(const QModelIndex &index,
              uint &portGroupIdx, uint &portIdx) const;

};

#endif
