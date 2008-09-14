#include "portstatsmodel.h"
#include "portgrouplist.h"

#include <QTimer>

PortStatsModel::PortStatsModel(PortGroupList *p, QObject *parent)
	: QAbstractTableModel(parent) 
{
	QTimer	*timer;

	pgl = p;

	timer = new QTimer();
	connect(timer, SIGNAL(timeout()), this, SLOT(updateStats()));
	timer->start(5000);
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

	// Check for a valid index
	if (!index.isValid())
		return QVariant();

	// Check for row/column limits
	if (index.row() >= e_STAT_MAX)
		return QVariant();

	if (numPorts.isEmpty())
		return QVariant();

	if (index.column() >= (numPorts.last()))
		return QVariant();

	getDomainIndexes(index, pgidx, pidx);

	// Check role
	if (role == Qt::DisplayRole)
	{
		OstProto::PortStats	stats;

		stats = pgl->mPortGroups.at(pgidx)->mPorts[pidx].getStats();

		switch(index.row())
		{
			case e_STAT_FRAMES_RCVD:
				return stats.rx_pkts();

			case e_STAT_FRAMES_SENT:
				return stats.tx_pkts();

			case e_STAT_FRAME_SEND_RATE:
				return stats.tx_pps();

			case e_STAT_FRAME_RECV_RATE:
				return stats.rx_pps();

			case e_STAT_BYTES_RCVD:
				return stats.rx_bytes();

			case e_STAT_BYTES_SENT:
				return stats.tx_bytes();

			case e_STAT_BYTE_SEND_RATE:
				return stats.tx_bps();

			case e_STAT_BYTE_RECV_RATE:
				return stats.rx_bps();

			default:
				return 0;
		}
	}
	else
		return QVariant();

}

QVariant PortStatsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return QString("Port %1").arg(section);
	else
		return PortStatName.at(section);
}

void PortStatsModel::portListFromIndex(QModelIndexList indices, 
		QList<PortGroupAndPortList> &portList)
{
	portList.clear();

	for (int i = 0; i < indices.size(); i++)
	{
		//getDomainIndexes(indices.at(i), portGroupIdx, portIdx);	

		for (int j = 0; j < portList.size(); j++)
		{
			// FIXME(HI): Incomplete!!!!
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

void PortStatsModel::on_portStatsUpdate(int port, void*stats)
{
	// FIXME(MED): update only the changed port not all
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

void PortStatsModel::when_portGroup_stats_update(quint32 portGroupId)
{
	// FIXME(MED): update only the changed ports, not all
	reset();
}
