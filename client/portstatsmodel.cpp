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

#include <QPainter>
#include <QPixmapCache>
#include <QTimer>

enum {
    // XXX: The byte stats don't include FCS so include it in the overhead
    kPerPacketByteOverhead = 24 // 1(SFD)+7(Preamble)+12(IPG)+4(FCS)
};

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
            portIdx = (portNum - 1) - numPorts.at(portGroupIdx - 1); 
        else
            portIdx = portNum - 1; 
    }
    else
        portIdx = portNum - 1; 

    //qDebug("PSM: %d - %d, %d", index.column(), portGroupIdx, portIdx);
}

QVariant PortStatsModel::data(const QModelIndex &index, int role) const
{
    // Check for a valid index
    if (!index.isValid())
        return QVariant();

    // Check for row/column limits
    int row = index.row();
    if (row >= e_STAT_MAX)
        return QVariant();

    if (numPorts.isEmpty())
        return QVariant();

    if (index.column() >= (numPorts.last()))
        return QVariant();

    if (role == Qt::TextAlignmentRole)
    {
        if (row >= e_STATISTICS_START && row <= e_STATISTICS_END)
            return Qt::AlignRight;      // right-align numbers
        else
            return Qt::AlignHCenter;    // center-align everything else
    }

    uint pgidx, pidx;
    getDomainIndexes(index, pgidx, pidx);

    OstProto::PortStats stats = pgl->mPortGroups.at(pgidx)
                                        ->mPorts[pidx]->getStats();
    // Check role
    if (role == Qt::DisplayRole)
    {
        switch(row)
        {
            // Info
            case e_INFO_USER:
                return pgl->mPortGroups.at(pgidx)->mPorts[pidx]->userName();

            // States
            case e_COMBO_STATE:
                return QVariant();

            // Statistics
            case e_STAT_FRAMES_RCVD:
                return QString("%L1").arg(quint64(stats.rx_pkts()));

            case e_STAT_FRAMES_SENT:
                return QString("%L1").arg(quint64(stats.tx_pkts()));

            case e_STAT_FRAME_SEND_RATE:
                return QString("%L1").arg(quint64(stats.tx_pps()));

            case e_STAT_FRAME_RECV_RATE:
                return QString("%L1").arg(quint64(stats.rx_pps()));

            case e_STAT_BYTES_RCVD:
                return QString("%L1").arg(quint64(stats.rx_bytes()));

            case e_STAT_BYTES_SENT:
                return QString("%L1").arg(quint64(stats.tx_bytes()));

            case e_STAT_BYTE_SEND_RATE:
                return QString("%L1").arg(quint64(stats.tx_bps()));

            case e_STAT_BYTE_RECV_RATE:
                return QString("%L1").arg(quint64(stats.rx_bps()));

            case e_STAT_BIT_SEND_RATE:
                return QString("%L1").arg(quint64(
                            stats.tx_bps()
                                + stats.tx_pps()*kPerPacketByteOverhead)*8);

            case e_STAT_BIT_RECV_RATE:
                return QString("%L1").arg(quint64(
                            stats.rx_bps()
                                + stats.rx_pps()*kPerPacketByteOverhead)*8);

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

            case e_STAT_RX_DROPS:
                return QString("%L1").arg(quint64(stats.rx_drops()));
            case e_STAT_RX_ERRORS:
                return QString("%L1").arg(quint64(stats.rx_errors()));
            case e_STAT_RX_FIFO_ERRORS:
                return QString("%L1").arg(quint64(stats.rx_fifo_errors()));
            case e_STAT_RX_FRAME_ERRORS:
                return QString("%L1").arg(quint64(stats.rx_frame_errors()));

            default:
                qWarning("%s: Unhandled stats id %d\n", __FUNCTION__,
                        index.row());
                return 0;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (row == e_COMBO_STATE)
            return statusIcons(
                        stats.state().link_state(),
                        stats.state().is_transmit_on(),
                        stats.state().is_capture_on());
        else
            return QVariant();
    }
    else if (role == Qt::ToolTipRole)
    {
        if (row == e_COMBO_STATE) {
            QString linkIcon;
            switch (stats.state().link_state()) {
                case OstProto::LinkStateUp:
                    linkIcon = ":/icons/bullet_green.png";
                    break;
                case OstProto::LinkStateDown:
                    linkIcon = ":/icons/bullet_red.png";
                    break;
                case OstProto::LinkStateUnknown:
                    linkIcon = ":/icons/bullet_white.png";
                    break;
            }
            // FIXME: Ideally, the text should be vertically centered wrt icon
            // but style='vertical-align:middle for the img tag doesn't work
            QString tooltip = QString("<img src='%1'/> Link %2")
                                .arg(linkIcon)
                                .arg(LinkStateName.at(
                                            stats.state().link_state()));
            if (stats.state().is_transmit_on())
                tooltip.prepend("<img src=':/icons/transmit_on.png'/>"
                                " Transmit On<br/>");
            if (stats.state().is_capture_on())
                tooltip.append("<br/><img src=':/icons/sound_none.png'/>"
                               " Capture On");
            return tooltip;
        }
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

            if (numPorts.isEmpty() || section >= numPorts.last())
                return QVariant();
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

        if (numPorts.isEmpty() || section >= numPorts.last())
            return QVariant();
        getDomainIndexes(index(0, section), portGroupIdx, portIdx);    
        portName = QString("Port %1-%2")
            .arg(pgl->mPortGroups.at(portGroupIdx)->id())
            .arg(pgl->mPortGroups.at(portGroupIdx)->mPorts.at(portIdx)->id());
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

    beginResetModel();

    // recalc numPorts
    while (numPorts.size())
        numPorts.removeFirst();

    for (i = 0; i < pgl->mPortGroups.size(); i++)
    {
        count += pgl->mPortGroups.at(i)->numPorts();
        numPorts.append(count);
    }

    endResetModel();
}

// FIXME: unused? if used, the index calculation row/column needs to be swapped
#if 0
void PortStatsModel::on_portStatsUpdate(int port, void* /*stats*/)
{
    QModelIndex topLeft = index(port, 0, QModelIndex());
    QModelIndex bottomRight = index(port, e_STAT_MAX, QModelIndex());

    emit dataChanged(topLeft, bottomRight);
}
#endif

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
    QModelIndex bottomRight = index(rowCount()-1, columnCount()-1, QModelIndex());

    emit dataChanged(topLeft, bottomRight);
}

QPixmap PortStatsModel::statusIcons(
        int linkState, bool transmit, bool capture) const
{
    QPixmap pixmap;
    QString key = QString("$ost:statusList:%1:%2:%3")
                    .arg(linkState).arg(transmit).arg(capture);

    if (QPixmapCache::find(key, pixmap))
        return pixmap;

    static int sz = QPixmap(":/icons/transmit_on.png").width();

    // Assume all icons are of same size and are square
    QPixmap blank(sz, sz);
    blank.fill(Qt::transparent);

    pixmap = QPixmap(sz*3, sz);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);

    painter.drawPixmap(0, 0,
            transmit ? QPixmap(":/icons/transmit_on.png") : blank);

    switch (linkState) {
    case OstProto::LinkStateUp:
        painter.drawPixmap(sz, 0, QPixmap(":/icons/bullet_green.png"));
        break;
    case OstProto::LinkStateDown:
        painter.drawPixmap(sz, 0, QPixmap(":/icons/bullet_red.png"));
        break;
    case OstProto::LinkStateUnknown:
        painter.drawPixmap(sz, 0, QPixmap(":/icons/bullet_white.png"));
        break;
    default:
        painter.drawPixmap(sz, 0, blank);
    }

    painter.drawPixmap(sz*2, 0,
            capture ? QPixmap(":/icons/sound_none.png") : blank);


    QPixmapCache::insert(key, pixmap);
    return pixmap;
}
