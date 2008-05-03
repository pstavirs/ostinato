#include "portlistmodel.h"

-----------------------
This file is not used
-----------------------

PortListModel::PortListModel(QObject *parent)
	: QAbstractItemModel(parent) 
{
	portList = new QList<PortGroup>;

#if 0 // FIXME: Dummy data only for testing; to be removed
	PortGroup	pg;

	pg.name = "A";
	pg.isLocal = TRUE;
	pg.numPorts = 3;
	pg.port = new Port[pg.numPorts];
	pg.port[0].portId = 0;
	pg.port[0].name = "A0";
	pg.port[0].desc = "a0a0a0a0a0a0";
	pg.port[1].portId = 1;
	pg.port[1].name = "A1";
	pg.port[1].desc = "a1a1a1a1a1a1";
	pg.port[2].portId = 2;
	pg.port[2].name = "A2";
	pg.port[2].desc = "a2a2a2a2a2a2";

	portList->append(pg);

	pg.name = "B";
	pg.isLocal = FALSE;
	pg.numPorts = 2;
	pg.port = new Port[pg.numPorts];
	pg.port[0].portId = 0;
	pg.port[0].name = "B0";
	pg.port[0].desc = "b0b0b0b0b0b0";
	pg.port[1].portId = 1;
	pg.port[1].name = "B1";
	pg.port[1].desc = "b1b1b1b1b1b1";

	portList->append(pg);
#endif
	// Do I need to do anything here?
}

int PortListModel::rowCount(const QModelIndex &parent) const
{
	// qDebug("RowCount Enter\n");
	if (!parent.isValid())
	{
		// Top Level Item
		// qDebug("RowCount top\n");
		// qDebug("RowCount Exit: %d\n", portList->size());
		return portList->size();
	}
		// qDebug("RowCount non top %d, %d, %llx\n", 
		// 	parent.row(), parent.column(), parent.internalId());

	quint16 p = parent.internalId() & 0xFFFF;
	if (p == 0xFFFF)
	{
		// qDebug("RowCount Exit: %d\n", portList->at(parent.row()).numPorts);
	return portList->at(parent.row()).numPorts;
	}
	else
	{
		// Leaf Item
		return 0;
	}
}

int PortListModel::columnCount(const QModelIndex &parent ) const
{
	return 1;	// FIXME: hardcoding
}

Qt::ItemFlags PortListModel::flags(const QModelIndex &index) const
{
	return QAbstractItemModel::flags(index); // FIXME: no need for this func
}
QVariant PortListModel::data(const QModelIndex &index, int role) const
{
	//qDebug("Enter PortListModel data\n");
	// Check for a valid index
	if (!index.isValid())
		return QVariant();


	// Check role
	if ((role == Qt::DisplayRole))
	{
#if 0 // Only for debug
		qDebug("Exit PortListModel data\n");
		return "Testing"; // FIXME: for dbg only
#endif

		QModelIndex	parent = index.parent();

		if (!parent.isValid())
		{
			// Top Level Item
			return QString("%1 (%2) [%3]").
				arg(portList->at(index.row()).name).
				arg(portList->at(index.row()).isLocal == TRUE? "LOCAL" : "REMOTE").
				arg(portList->at(index.row()).numPorts); 
		}
		return QString("%1: %2 (%3)").
			arg(portList->at(parent.row()).port[index.row()].portId).
			arg(portList->at(parent.row()).port[index.row()].name).
			arg(portList->at(parent.row()).port[index.row()].desc);
	}
	else
		return QVariant();
}

QVariant PortListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return QVariant();
	else
		return QString("%1").arg(section+1);
}

QModelIndex PortListModel::index (int row, int col, 
	const QModelIndex & parent) const
{
#if 0
	if (!hasIndex(row, col, parent))
		return QModelIndex();
#endif
	
	//qDebug("index: R=%d, C=%d, PR=%d, PC=%d, PID=%llx\n",
	//	row, col, parent.row(), parent.column(), parent.internalId());

	if (!parent.isValid())
	{
		// Top Level Item
		quint16 pg = row, p = 0xFFFF;
		quint32 id = (pg << 16) | p;
		//qDebug("index (top) dbg: PG=%d, P=%d, ID=%x\n", pg, p, id);

		return createIndex(row, col, id);
	}
	else
	{
		quint16 pg = parent.row(), p = row;
		quint32 id = (pg << 16) | p;
		//qDebug("index (nontop) dbg: PG=%d, P=%d, ID=%x\n", pg, p, id);

		return createIndex(row, col, id);
	}
}

QModelIndex PortListModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();
	
	//qDebug("parent: R=%d, C=%d ID=%llx\n",
	//	index.row(), index.column(), index.internalId());

	quint16 pg = index.internalId() >> 16;
	quint16 p = index.internalId() & 0x0000FFFF;

	//qDebug("parent dbg: PG=%d, P=%d\n", pg, p);

	if (p == 0xFFFF)
	{
		//qDebug("parent ret: NULL\n");
		// Top Level Item - PG
		return QModelIndex();
	}

	quint32 id = (pg << 16) | 0xFFFF;
	//qDebug("parent ret: R=%d, C=%d, ID=%x\n",
	//	pg, 0, id);

	return createIndex(pg, 0, id);

}

void PortListModel::doRefresh()
{
	emit layoutChanged();
}


