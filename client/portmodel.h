#ifndef _PORT_MODEL_H
#define _PORT_MODEL_H

#include <QAbstractItemModel>

class PortGroupList;
class PortGroup;

class PortModel : public QAbstractItemModel
{
	Q_OBJECT

	friend class PortGroupList;

	PortGroupList	*pgl;

	public:
		PortModel(PortGroupList *p, QObject *parent = 0);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
		QModelIndex parent(const QModelIndex &index) const;	

		bool isPortGroup(const QModelIndex& index);
		bool isPort(const QModelIndex& index);
		quint32 portGroupId(const QModelIndex& index);
		quint32 portId(const QModelIndex& index);


private slots:
	void when_portGroupDataChanged(PortGroup *portGroup);

	void portGroupAboutToBeAppended();
	void portGroupAppended();
	void portGroupAboutToBeRemoved(PortGroup *portGroup);
	void portGroupRemoved();

	void when_portListChanged();

#if 0
	void triggerLayoutAboutToBeChanged();
	void triggerLayoutChanged();
#endif

};

#endif
