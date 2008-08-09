#include "streammodel.h"
#include "portgrouplist.h"
#include "qicon.h"

StreamModel::StreamModel(PortGroupList *p, QObject *parent)
	: QAbstractTableModel(parent) 
{
	pgl = p;
	mCurrentPort = NULL;
}

int StreamModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	if (mCurrentPort)
		return mCurrentPort->mStreams.size();
	else
		return 0;
}

int StreamModel::columnCount(const QModelIndex &parent ) const
{
	return (int) StreamMaxFields;
}

Qt::ItemFlags StreamModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags	flags = QAbstractTableModel::flags(index);


	switch (index.column())
	{
	case StreamIcon:
		break;
	case StreamName:
		flags |= Qt::ItemIsEditable;
		break;
	case StreamStatus:
		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
		break;
	default:
		break;
	}

	return flags;
}
QVariant StreamModel::data(const QModelIndex &index, int role) const
{
	// Check for a valid index
	if (!index.isValid())
		return QVariant();

	// Check for row/column limits
	if (index.row() >= mCurrentPort->mStreams.size())
		return QVariant();

	if (index.column() >= StreamMaxFields)
		return QVariant();

	if (mCurrentPort == NULL)
		return QVariant();

	// Return data based on field and role
	switch(index.column())
	{
		case StreamIcon:
		{
			if (role == Qt::DecorationRole)
				return QIcon(":/icons/stream_edit.png");
#if 0
			else if ((role == Qt::DisplayRole))
				return QString("EDIT");
#endif
			else
				return QVariant();
			break;
		}
		case StreamName:
		{
			if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
				return mCurrentPort->mStreams[index.row()].name();
			else
				return QVariant();
			break;
		}
		case StreamStatus:
		{
	#if 0
			if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
				return streamList[index.row()].isEnabled;
			if ((role == Qt::DisplayRole) || (role == Qt::CheckStateRole) || 
	#endif
			if ((role == Qt::CheckStateRole) || (role == Qt::EditRole))
			{
				if (mCurrentPort->mStreams[index.row()].isEnabled())
					return Qt::Checked;
				else
					return Qt::Unchecked;
			}
			else
				return QVariant();
			break;
		}
		default:
			qDebug("-------------UNHANDLED STREAM FIELD----------------");
	}

	return QVariant();
}

bool StreamModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (mCurrentPort == NULL)
		return false;

	if (index.isValid() && role == Qt::EditRole) 
	{
		// Edit Supported Fields
		switch (index.column())
		{
		case StreamName:
			mCurrentPort->mStreams[index.row()].setName(value.toString());
			return true;
			break;
		case StreamStatus:
			mCurrentPort->mStreams[index.row()].setIsEnabled(value.toBool());
			return true;
			break;

		// Edit Not Supported Fields
		case StreamIcon:
			return false;
			break;

		// Unhandled Stream Field
		default:
			qDebug("-------------UNHANDLED STREAM FIELD----------------");
			break;
		}
	}
	return false;
}

QVariant StreamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
	{
		switch(section)
		{
		case StreamIcon:
			return QString("Icon");
			break;
		case StreamName:
			return QString("Name");
			break;
		case StreamStatus:
			return QString("Enabled");
			break;
		default:
			qDebug("-------------UNHANDLED STREAM FIELD----------------");
			break;
		}
	}
	else
		return QString("%1").arg(section+1);

	return QVariant();
}

bool StreamModel::insertRows(int row, int count, const QModelIndex &parent) 
{
	qDebug("insertRows() row = %d", row);
	qDebug("insertRows() count = %d", count);
	beginInsertRows(QModelIndex(), row, row+count-1);
	for (int i = 0; i < count; i++)
		mCurrentPort->mStreams.insert(row, Stream());
	endInsertRows();

	return true;
}

bool StreamModel::removeRows(int row, int count, const QModelIndex &parent) 
{
	qDebug("removeRows() row = %d", row);
	qDebug("removeRows() count = %d", count);
	beginRemoveRows(QModelIndex(), row, row+count-1);
	for (int i = 0; i < count; i++)
	{
		// FIXME(HIGH): do we need to free the removed stream?
		mCurrentPort->mStreams.removeAt(row);
	}
	endRemoveRows();

	return true;
}

// --------------------- SLOTS ------------------------

void StreamModel::setCurrentPortIndex(const QModelIndex &current)
{
	if (!current.isValid() || !pgl->isPort(current))
	{
		qDebug("current is either invalid or not a port");
		mCurrentPort = NULL;
	}
	else
	{
		qDebug("change to valid port");
		quint16 pg = current.internalId() >> 16;
		mCurrentPort = &pgl->mPortGroups[pgl->indexOfPortGroup(pg)]->mPorts[current.row()];
	}
	reset();
}
