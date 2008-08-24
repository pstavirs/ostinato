#include "portstatsmodel.h"
#include "portgrouplist.h"

PortStatsModel::PortStatsModel(PortGroupList *p, QObject *parent)
	: QAbstractTableModel(parent) 
{
	pgl = p;
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

QVariant PortStatsModel::data(const QModelIndex &index, int role) const
{
	int pgidx, pidx, portNum;

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

	// TODO(LOW): Optimize using binary search: see qLowerBound()
	portNum = index.column() + 1;
	for (pgidx = 0; pgidx < numPorts.size(); pgidx++)
		if (portNum <= numPorts.at(pgidx))
			break;

	if (pgidx)
	{
		if (numPorts.at(pgidx -1))
			pidx = (portNum - 1) % numPorts.at(pgidx - 1); 
		else
			pidx = portNum - 1; 
	}
	else
		pidx = portNum - 1; 

	//qDebug("PSM: %d - %d, %d", index.column(), pgidx, pidx);

	// Check role
	if (role == Qt::DisplayRole)
	{
#if 0 // PB
		return pgl->mPortGroups.at(pgidx)->mPorts.at(pidx).mPortStats[index.row()];
#endif	
		return 0;	//FIXME: Get actual port stats
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


