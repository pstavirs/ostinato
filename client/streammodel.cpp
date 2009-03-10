#include "stream.h"
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
		return mCurrentPort->numStreams();
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
		flags |= Qt::ItemIsUserCheckable;
		break;
	case StreamNextWhat:
		flags |= Qt::ItemIsEditable;
		break;
	default:
		//qFatal("Missed case in switch!");
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
	if (index.row() >= mCurrentPort->numStreams())
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
			else
				return QVariant();
			break;
		}
		case StreamName:
		{
			if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
				return mCurrentPort->streamByIndex(index.row()).name();
			else
				return QVariant();
			break;
		}
		case StreamStatus:
		{
			if ((role == Qt::CheckStateRole))
			{
				if (mCurrentPort->streamByIndex(index.row()).isEnabled())
					return Qt::Checked;
				else
					return Qt::Unchecked;
			}
			else
				return QVariant();
			break;
		}
		case StreamNextWhat:
		{
			int val = mCurrentPort->streamByIndex(index.row()).nextWhat();

			if (role == Qt::DisplayRole)
				return nextWhatOptionList().at(val);
			else if	(role == Qt::EditRole)
				return val;
			else
				return QVariant();

			break;
		}
		default:
			qFatal("-------------UNHANDLED STREAM FIELD----------------");
	}

	return QVariant();
}

bool StreamModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (mCurrentPort == NULL)
		return false;

	if (index.isValid()) 
	{
		switch (index.column())
		{
		// Edit Supported Fields
		case StreamName:
			mCurrentPort->streamByIndex(index.row()).setName(value.toString());
			emit(dataChanged(index, index));
			return true;

		case StreamStatus:
			mCurrentPort->streamByIndex(index.row()).setIsEnabled(value.toBool());
			emit(dataChanged(index, index));
			return true;

		case StreamNextWhat:
			if (role == Qt::EditRole)
			{	
				mCurrentPort->streamByIndex(index.row()).setNextWhat(
						(Stream::NextWhat)value.toInt());
				emit(dataChanged(index, index));
				return true;
			}
			else 
				return false;

		// Edit Not Supported Fields
		case StreamIcon:
			return false;

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
			return QString("");
			break;
		case StreamName:
			return QString("Name");
			break;
		case StreamStatus:
			return QString("");
			break;
		case StreamNextWhat:
			return QString("Goto");
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
		mCurrentPort->newStreamAt(row);
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
		mCurrentPort->deleteStreamAt(row);
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
