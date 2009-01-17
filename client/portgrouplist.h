#ifndef _PORT_GROUP_LIST_H
#define _PORT_GROUP_LIST_H

#include "portgroup.h"
#include <QAbstractItemModel>
#include <QItemSelection>
#include "portmodel.h"
#include "streammodel.h"
#include "portstatsmodel.h"

class PortModel;
class StreamModel;

class PortGroupList : public QObject {

	Q_OBJECT

	friend class PortModel;
	friend class StreamModel;
	friend class PortStatsModel;

	QList<PortGroup*>	mPortGroups;	
	PortModel			mPortGroupListModel;
	StreamModel			mStreamListModel;
	PortStatsModel		mPortStatsModel;

// Methods
public:
	PortGroupList();


	PortModel* getPortModel() { return &mPortGroupListModel; }
	PortStatsModel* getPortStatsModel() { return &mPortStatsModel; }
	StreamModel* getStreamModel() { return &mStreamListModel; }

	bool isPortGroup(const QModelIndex& index);
	bool isPort(const QModelIndex& index);
	PortGroup& portGroup(const QModelIndex& index);
	Port& port(const QModelIndex& index);

	int numPortGroups() { return mPortGroups.size(); }
	PortGroup& portGroupByIndex(int index) { return *(mPortGroups[index]); }

	void addPortGroup(PortGroup &portGroup);
	void removePortGroup(PortGroup &portGroup);

private:
	int indexOfPortGroup(quint32 portGroupId);

};

#endif
