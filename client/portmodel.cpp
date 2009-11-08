#include "portmodel.h"
#include "portgrouplist.h"
#include <QIcon>

#if 0
#define DBG0(x)	qDebug(x)
#define DBG1(x, p1)	qDebug(x, (p1))
#else
#define DBG0(x)	{} 
#define DBG1(x, p1)	{} 
#endif

PortModel::PortModel(PortGroupList *p, QObject *parent)
	: QAbstractItemModel(parent) 
{
	pgl = p;
}

int PortModel::rowCount(const QModelIndex &parent) const
{
	// qDebug("RowCount Enter\n");
	if (!parent.isValid())
	{
		// Top Level Item
		//qDebug("RowCount (Top) Exit: %d\n", pgl->mPortGroups.size());
		return pgl->mPortGroups.size();
	}
		// qDebug("RowCount non top %d, %d, %llx\n", 
		// 	parent.row(), parent.column(), parent.internalId());

	quint16 pg = (parent.internalId() >> 16) & 0xFFFF;
	quint16 p = parent.internalId() & 0xFFFF;
	if (p == 0xFFFF)
	{
#if 0 // wrong code?
		int count = 0;
		foreach(PortGroup *pg, pgl->mPortGroups)
		{
			count += pg->numPorts();
		}
		//qDebug("RowCount (Mid) Exit: %d\n", count);
		return count;
#endif
		if (parent.column() == 0)
			return pgl->mPortGroups.value(pgl->indexOfPortGroup(pg))->numPorts(); 
		else
			return 0;
	}
	else
	{
		// Leaf Item
		return 0;
	}
}

int PortModel::columnCount(const QModelIndex &parent ) const
{
	return 1;	// FIXME: hardcoding
}

Qt::ItemFlags PortModel::flags(const QModelIndex &index) const
{
	return QAbstractItemModel::flags(index); // FIXME: no need for this func
}
QVariant PortModel::data(const QModelIndex &index, int role) const
{

	DBG0("Enter PortModel data\n");

	// Check for a valid index
	if (!index.isValid())
		return QVariant();

	DBG1("PortModel::data(index).row = %d", index.row());
	DBG1("PortModel::data(index).column = %0d", index.column());
	DBG1("PortModel::data(index).internalId = %08llx", index.internalId());

	QModelIndex	parent = index.parent();

	if (!parent.isValid())
	{
		// Top Level Item - PortGroup
		if ((role == Qt::DisplayRole))
		{
			DBG0("Exit PortModel data 1\n");
			return QString("Port Group %1: %2 [%3:%4] (%5)").
				arg(pgl->mPortGroups.at(index.row())->id()).
				arg(pgl->mPortGroups.at(index.row())->userAlias()).
				arg(pgl->mPortGroups.at(index.row())->serverAddress().toString()).
				arg(pgl->mPortGroups.at(index.row())->serverPort()).
				arg(pgl->mPortGroups.value(index.row())->numPorts()); 
		}
		else if ((role == Qt::DecorationRole))
		{
			DBG0("Exit PortModel data 2\n");
			switch(pgl->mPortGroups.at(index.row())->state())
			{
				case QAbstractSocket::UnconnectedState:
					return QIcon(":/icons/bullet_red.png");

				case QAbstractSocket::HostLookupState:
					return QIcon(":/icons/bullet_yellow.png");

				case QAbstractSocket::ConnectingState:
				case QAbstractSocket::ClosingState:
					return QIcon(":/icons/bullet_orange.png");

				case QAbstractSocket::ConnectedState:
					return QIcon(":/icons/bullet_green.png");


				case QAbstractSocket::BoundState:
				case QAbstractSocket::ListeningState:
				default:
					return QIcon(":/icons/bullet_error.png");
			}
		}
		else 
		{
			DBG0("Exit PortModel data 3\n");
			return QVariant();
		}
	}
	else
	{
		// Non Top Level - Port
		if ((role == Qt::DisplayRole))
		{
			DBG0("Exit PortModel data 4\n");
			if (pgl->mPortGroups.at(parent.row())->numPorts() == 0)
				return QVariant();

			return QString("Port %1: %2 [%3] (%4)").
				arg(pgl->mPortGroups.at(
					parent.row())->mPorts[index.row()].id()).
				arg(pgl->mPortGroups.at(
					parent.row())->mPorts[index.row()].name()).
				arg(QHostAddress("0.0.0.0").toString()).	// FIXME(LOW)
				arg(pgl->mPortGroups.at(
					parent.row())->mPorts[index.row()].description());
		}
		else if ((role == Qt::DecorationRole))
		{
			DBG0("Exit PortModel data 5\n");
			if (pgl->mPortGroups.at(parent.row())->numPorts() == 0)
				return QVariant();
			switch(pgl->mPortGroups.at(parent.row())->mPorts[index.row()].linkState())
			{
				case OstProto::LinkStateUnknown:
					return QIcon(":/icons/bullet_white.png");
				case OstProto::LinkStateDown:
					return QIcon(":/icons/bullet_red.png");
				case OstProto::LinkStateUp:
					return QIcon(":/icons/bullet_green.png");
				default:
					qFatal("unexpected/unimplemented port oper state");
			}
		}
		else
		{
			DBG0("Exit PortModel data 6\n");
			return QVariant();
		}
	}

	return QVariant();
}

QVariant PortModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return QVariant();
	else
		return QString("Name");
}

QModelIndex PortModel::index (int row, int col, 
	const QModelIndex & parent) const
{
	if (!hasIndex(row, col, parent))
		return QModelIndex();
	
	//qDebug("index: R=%d, C=%d, PR=%d, PC=%d, PID=%llx\n",
	//	row, col, parent.row(), parent.column(), parent.internalId());

	if (!parent.isValid())
	{
		// Top Level Item
		quint16 pg =  pgl->mPortGroups.value(row)->id(), p = 0xFFFF;
		quint32 id = (pg << 16) | p;
		//qDebug("index (top) dbg: PG=%d, P=%d, ID=%x\n", pg, p, id);

		return createIndex(row, col, id);
	}
	else
	{
		quint16 pg = parent.internalId() >> 16;
		quint16 p = pgl->mPortGroups.value(parent.row())->mPorts.value(row).id();
		quint32 id = (pg << 16) | p;
		//qDebug("index (nontop) dbg: PG=%d, P=%d, ID=%x\n", pg, p, id);

		return createIndex(row, col, id);
	}
}

QModelIndex PortModel::parent(const QModelIndex &index) const
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
	//qDebug("parent ret: R=%d, C=%d, ID=%x\n", pg, 0, id);

	return createIndex(pgl->indexOfPortGroup(pg), 0, id);

}

bool PortModel::isPortGroup(const QModelIndex& index)
{
	if ((index.internalId() & 0xFFFF) == 0xFFFF)
		return true;
	else
		return false;
}

bool PortModel::isPort(const QModelIndex& index)
{
	if ((index.internalId() & 0xFFFF) != 0xFFFF)
		return true;
	else
		return false;
}

quint32 PortModel::portGroupId(const QModelIndex& index)
{
	return (index.internalId()) >> 16 & 0xFFFF;
}

quint32 PortModel::portId(const QModelIndex& index)
{
	return (index.internalId()) & 0xFFFF;
}



// ----------------------------------------------
// Slots
// ----------------------------------------------
void PortModel::when_portGroupDataChanged(PortGroup* portGroup, int portId)
{
	QModelIndex index;

	if (!pgl->mPortGroups.contains(portGroup))
	{
		qDebug("when_portGroupDataChanged(): pg not in list - do nothing");
		return;
	}

	qDebug("when_portGroupDataChanged pgid = %d", portGroup->id());
	qDebug("when_portGroupDataChanged idx = %d", pgl->mPortGroups.indexOf(portGroup));
	
	index = createIndex(pgl->mPortGroups.indexOf(portGroup), 0, 
		(portGroup->id() << 16) | portId);
	emit dataChanged(index, index);
}

void PortModel::portGroupAboutToBeAppended()
{
	int row;

	row = pgl->mPortGroups.size();
	beginInsertRows(QModelIndex(), row, row);
}

void PortModel::portGroupAppended()
{
	endInsertRows();
}

void PortModel::portGroupAboutToBeRemoved(PortGroup *portGroup)
{
	int row;

	row = pgl->mPortGroups.indexOf(portGroup);
	beginRemoveRows(QModelIndex(), row, row);
}

void PortModel::portGroupRemoved()
{
	endRemoveRows();
}

#if 0
void PortModel::triggerLayoutAboutToBeChanged()
{
	emit layoutAboutToBeChanged();
}

void PortModel::triggerLayoutChanged()
{
	emit layoutChanged();
}
#endif

void PortModel::when_portListChanged()
{
	reset();
}
