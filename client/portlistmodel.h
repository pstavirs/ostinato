#ifndef _PORT_LIST_MODEL_H
#define _PORT_LIST_MODEL_H


 -----------------
 This file is not used
 ------------------

#include <QAbstractItemModel>
#include <QStringList>
#include "portgroup.h"

static QStringList PortListCols = (QStringList()
	<< "Name"
);

class PortListModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
		//PortListModel(QObject *parent = 0) : QAbstractItemModel(parent) {}
		PortListModel(QObject *parent = 0);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
		QModelIndex parent(const QModelIndex &index) const;	

		void doRefresh();	// FIXME: temp till model exports functions to insert rows
		QList<PortGroup>	*portList; // FIXME: this shd be private
};

#endif
