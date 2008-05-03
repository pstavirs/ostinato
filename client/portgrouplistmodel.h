#ifndef _PORT_GROUP_LIST_H
#define _PORT_GROUP_LIST_H

#include "portgroup.h"
#include <QAbstractItemModel>

/* -----------------------------------------------------------
** NOTE: FILE NOT USED 'COZ MOC DOESN'T SUPPORT NESTED CLASSES
*-------------------------------------------------------------*/


class PortGroupList;

class PortGroupList : public QObject {

	Q_OBJECT

	class PortListModel : public QAbstractItemModel
	{
		// This is currently a read only model
		Q_OBJECT

		PortGroupList	*pgl;	// FIXME(HIGH): rename member

		public:
			PortListModel(PortGroupList *p, QObject *parent = 0);

			int rowCount(const QModelIndex &parent = QModelIndex()) const;
			int columnCount(const QModelIndex &parent = QModelIndex()) const;
			Qt::ItemFlags flags(const QModelIndex &index) const;
			QVariant data(const QModelIndex &index, int role) const;
			QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
			QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
			QModelIndex parent(const QModelIndex &index) const;	

		friend class PortGroupList; // FIXME(MED): Review need for friend
	};

	friend class PortListModel;

	QList<PortGroup*>	mPortGroups;	
	PortListModel		mPortGroupListModel;

// Methods
public:
	PortGroupList::PortGroupList();
	QAbstractItemModel* getModel();
	int addPortGroup(PortGroup &portGroup);
	int removePortGroup(PortGroup &portGroup);

private slots:
	void when_portlist_dataChanged();
};

#endif
