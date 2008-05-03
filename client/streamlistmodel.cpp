#include "streamlistmodel.h"

StreamListModel::StreamListModel(QObject *parent)
	: QAbstractTableModel(parent) 
{
	uint i;

	// Enable all streams by default
	for (i=0; i<MAX_ROWS; i++)
	{
		streamList[i].isEnabled = true;
	}
}

int StreamListModel::rowCount(const QModelIndex &parent) const
{
	return MAX_ROWS;	// FIXME
}

int StreamListModel::columnCount(const QModelIndex &parent ) const
{
	return MAX_COLS;	// FIXME
}

Qt::ItemFlags StreamListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags	flags = QAbstractTableModel::flags(index);

	if (index.column() == 0)
		;
	else if (index.column() == 1)
		flags |= Qt::ItemIsEditable;
	else if (index.column() == 2)
		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;

	return flags;
}
QVariant StreamListModel::data(const QModelIndex &index, int role) const
{
	// Check for a valid index
	if (!index.isValid())
		return QVariant();

	// Check for row/column limits
	if (index.row() >= MAX_ROWS)
		return QVariant();

	if (index.column() >= MAX_COLS)
		return QVariant();

	// Check role
	if (index.column() == 0) // Icon
	{
		if ((role == Qt::DisplayRole))
			return QString("EDIT");
		else
			return QVariant();
	}
	else if (index.column() == 1) // Name
	{
		if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
			return streamList[index.row()].streamName;
		else
			return QVariant();
	}
	else if (index.column() == 2) // Enabled?
	{
		//if ((role == Qt::CheckStateRole) || (role == Qt::EditRole))
			// return streamList[index.row()].isEnabled ? Qt::Checked : Qt::Unchecked;
		if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
			return streamList[index.row()].isEnabled;
		else
			return QVariant();
	}
}

bool StreamListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && role == Qt::EditRole) 
	{
		if (index.column() == 1) // Name
			streamList[index.row()].streamName = value.toString();
		else if (index.column() == 2) // Enabled?
			streamList[index.row()].isEnabled = value.toBool();
		else
			return false;

		return true;
	}
	return false;
}

QVariant StreamListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return StreamListCols.at(section);
	else
		return QString("%1").arg(section+1);
}

#if 0
// QModelIndex StreamListModel::index (int portNum, PortStat stat, const QModelIndex & parent = QModelIndex() ) const

void StreamListModel::on_portStatsUpdate(int port, void*stats)
{
	int i;
	QModelIndex topLeft = index(port, 0, QModelIndex());
	QModelIndex bottomRight = index(port, e_STAT_MAX, QModelIndex());

	for (i = 0; i < e_STAT_MAX; i++)
		dummyStats[port][i] = ((int *)stats)[i];

	emit dataChanged(topLeft, bottomRight);
}

#endif
