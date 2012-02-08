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

#include "portstatsmodel.h"
#include "portgrouplist.h"

#include <QTimer>

PortStatsModel::PortStatsModel(PortGroupList *p, QObject *parent)
    : QAbstractTableModel(parent) 
{
    pgl = p;

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(updateStats()));
    timer->start(1000);
}

PortStatsModel::~PortStatsModel()
{
    timer->stop();
    delete timer;
}

int PortStatsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (numPorts.isEmpty())
        return 0;

    if (numPorts.last() == 0)
        return 0;

    return (int) e_STAT_MAX;    
}

int PortStatsModel::columnCount(const QModelIndex &parent ) const
{
    if (parent.isValid())
        return 0;
    else
        if (numPorts.isEmpty())
            return 0;
        else
            return numPorts.last();
}

void PortStatsModel::getDomainIndexes(const QModelIndex &index,
        uint &portGroupIdx, uint &portIdx) const
{
    int portNum;

    // TODO(LOW): Optimize using binary search: see qLowerBound()
    portNum = index.column() + 1;
    for (portGroupIdx = 0; portGroupIdx < (uint) numPorts.size(); portGroupIdx++)
        if (portNum <= numPorts.at(portGroupIdx))
            break;

    if (portGroupIdx)
    {
        if (numPorts.at(portGroupIdx -1))
            portIdx = (portNum - 1) % numPorts.at(portGroupIdx - 1); 
        else
            portIdx = portNum - 1; 
    }
    else
        portIdx = portNum - 1; 

    //qDebug("PSM: %d - %d, %d", index.column(), portGroupIdx, portIdx);
}

QVariant PortStatsModel::data(const QModelIndex &index, int role) const
{
    uint pgidx, pidx;
    int row;

    // Check for a valid index
    if (!index.isValid())
        return QVariant();

    // Check for row/column limits
    row = index.row();
    if (row >= e_STAT_MAX)
        return QVariant();

    if (numPorts.isEmpty())
        return QVariant();

    if (index.column() >= (numPorts.last()))
        return QVariant();

    getDomainIndexes(index, pgidx, pidx);

    // Check role
    if (role == Qt::DisplayRole)
    {
        OstProto::PortStats    stats;

        stats = pgl->mPortGroups.at(pgidx)->mPorts[pidx]->getStats();

        switch(row)
        {
            // States
            case e_LINK_STATE:
                return LinkStateName.at(stats.state().link_state());

            case e_TRANSMIT_STATE:
                return BoolStateName.at(stats.state().is_transmit_on());

            case e_CAPTURE_STATE:
                return BoolStateName.at(stats.state().is_capture_on());

            // Statistics
            case e_STAT_FRAMES_RCVD:
                return quint64(stats.rx_pkts());

            case e_STAT_FRAMES_SENT:
                return quint64(stats.tx_pkts());

            case e_STAT_FRAME_SEND_RATE:
                return quint64(stats.tx_pps());

            case e_STAT_FRAME_RECV_RATE:
                return quint64(stats.rx_pps());

            case e_STAT_BYTES_RCVD:
                return quint64(stats.rx_bytes());

            case e_STAT_BYTES_SENT:
                return quint64(stats.tx_bytes());

            case e_STAT_BYTE_SEND_RATE:
                return quint64(stats.tx_bps());

            case e_STAT_BYTE_RECV_RATE:
                return quint64(stats.rx_bps());

#if 0
            case e_STAT_FRAMES_RCVD_NIC:
                return stats.rx_pkts_nic();

            case e_STAT_FRAMES_SENT_NIC:
                return stats.tx_pkts_nic();

            case e_STAT_BYTES_RCVD_NIC:
                return stats.rx_bytes_nic();

            case e_STAT_BYTES_SENT_NIC:
                return stats.tx_bytes_nic();
#endif
            case e_STAT_RX_DROPS : return quint64(stats.rx_drops());
            case e_STAT_RX_ERRORS: return quint64(stats.rx_errors());
            case e_STAT_RX_FIFO_ERRORS: return quint64(stats.rx_fifo_errors());
            case e_STAT_RX_FRAME_ERRORS: return quint64(stats.rx_frame_errors());

            default:
                qWarning("%s: Unhandled stats id %d\n", __FUNCTION__,
                        index.row());
                return 0;
        }
    }
    else if (role == Qt::TextAlignmentRole) 
    {
        if (row >= e_STATE_START && row <= e_STATE_END)
            return Qt::AlignHCenter;
        else if (row >= e_STATISTICS_START && row <= e_STATISTICS_END)
            return Qt::AlignRight;
        else
            return QVariant();
    }
    else
        return QVariant();

}

QVariant PortStatsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::ToolTipRole)
    {
        if (orientation == Qt::Horizontal)
        {
            QString notes;
            uint portGroupIdx, portIdx;

            getDomainIndexes(index(0, section), portGroupIdx, portIdx);    
            notes = pgl->mPortGroups.at(portGroupIdx)->mPorts[portIdx]->notes();
            if (!notes.isEmpty())
                return notes;
            else
                return QVariant();
        }
        else
            return QVariant();
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        uint portGroupIdx, portIdx;
        QString portName;

        getDomainIndexes(index(0, section), portGroupIdx, portIdx);    
        portName = QString("Port %1-%2").arg(portGroupIdx).arg(portIdx);
        if (portGroupIdx < (uint) pgl->mPortGroups.size() 
            && portIdx < (uint) pgl->mPortGroups.at(portGroupIdx)->mPorts.size())
        {
            if (!pgl->mPortGroups.at(portGroupIdx)->mPorts[portIdx]->notes()
                    .isEmpty())
                portName += " *";
        }
        return portName;
    }
    else
        return PortStatName.at(section);
}

void PortStatsModel::portListFromIndex(QModelIndexList indices, 
        QList<PortGroupAndPortList> &portList)
{
    int i, j;
    QModelIndexList        selectedCols(indices);

    portList.clear();

    //selectedCols = indices.selectedColumns();
    for (i = 0; i < selectedCols.size(); i++)
    {
        uint portGroupIdx, portIdx;

        getDomainIndexes(selectedCols.at(i), portGroupIdx, portIdx);    
        for (j = 0; j < portList.size(); j++)
        {
            if (portList[j].portGroupId == portGroupIdx)
                break;
        }

        if (j >= portList.size())
        {
            // PortGroup Not found
            PortGroupAndPortList    p;

            p.portGroupId = portGroupIdx;
            p.portList.append(portIdx);

            portList.append(p);
        }
        else
        {
            // PortGroup found

            portList[j].portList.append(portIdx);
        }
    }
}

//
// Slots
//
void PortStatsModel::when_portListChanged()
{
    int i, count = 0;

    // recalc numPorts
    while (numPorts.size())
        numPorts.removeFirst();

    for (i = 0; i < pgl->mPortGroups.size(); i++)
    {
        count += pgl->mPortGroups.at(i)->numPorts();
        numPorts.append(count);
    }

    reset();
}

void PortStatsModel::on_portStatsUpdate(int port, void* /*stats*/)
{
    QModelIndex topLeft = index(port, 0, QModelIndex());
    QModelIndex bottomRight = index(port, e_STAT_MAX, QModelIndex());

    emit dataChanged(topLeft, bottomRight);
}

void PortStatsModel::updateStats()
{
    // Request each portgroup to fetch updated stats - the port group
    // raises a signal once updated stats are available
    for (int i = 0; i < pgl->mPortGroups.size(); i++)
        pgl->mPortGroups[i]->getPortStats();
}

void PortStatsModel::when_portGroup_stats_update(quint32 /*portGroupId*/)
{
    // FIXME(MED): update only the changed ports, not all

    QModelIndex topLeft = index(0, 0, QModelIndex());
    QModelIndex bottomRight = index(rowCount(), columnCount(), QModelIndex());

    emit dataChanged(topLeft, bottomRight);
}
