#include "portgrouplist.h"

// TODO(LOW): Remove
#include <modeltest.h>

PortGroupList::PortGroupList()
	: mPortGroupListModel(this), 
	  mStreamListModel(this),
	  mPortStatsModel(this, this)
{
	PortGroup	*pg;

	// TODO(LOW): Remove
	new ModelTest(getStreamModel());
	new ModelTest(getPortModel());
	new ModelTest(getPortStatsModel());
	
	// Add the "Local" Port Group
	pg = new PortGroup;
	addPortGroup(*pg);
}

bool PortGroupList::isPortGroup(const QModelIndex& index)
{
	return mPortGroupListModel.isPortGroup(index);
}

bool PortGroupList::isPort(const QModelIndex& index)
{
	return mPortGroupListModel.isPort(index);
}

PortGroup& PortGroupList::portGroup(const QModelIndex& index)
{
	if (mPortGroupListModel.isPortGroup(index))
	{
		//return *(mPortGroups[mPortGroupListModel.portGroupId(index)]);
		return *(mPortGroups[index.row()]);
	}
#if 0 // FIXME(MED)
	else
		return NULL;
#endif
}

Port& PortGroupList::port(const QModelIndex& index)
{
	if (mPortGroupListModel.isPort(index))
	{
		return (mPortGroups.at(index.parent().row())->
			mPorts[index.row()]);
	}
#if 0 // FIXME(MED)
	else
		return NULL;
#endif
}

void PortGroupList::addPortGroup(PortGroup &portGroup)
{
	mPortGroupListModel.portGroupAboutToBeAppended();

	connect(&portGroup, SIGNAL(portGroupDataChanged(PortGroup*)),
		&mPortGroupListModel, SLOT(when_portGroupDataChanged(PortGroup*)));
#if 0
	connect(&portGroup, SIGNAL(portListAboutToBeChanged(quint32)),
		&mPortGroupListModel, SLOT(triggerLayoutAboutToBeChanged()));
	connect(&portGroup, SIGNAL(portListChanged(quint32)),
		&mPortGroupListModel, SLOT(triggerLayoutChanged()));
#endif
	connect(&portGroup, SIGNAL(portListChanged(quint32)),
		&mPortGroupListModel, SLOT(when_portListChanged()));

	connect(&portGroup, SIGNAL(portListChanged(quint32)),
		&mPortStatsModel, SLOT(when_portListChanged()));

	connect(&portGroup, SIGNAL(statsChanged(quint32)),
		&mPortStatsModel, SLOT(when_portGroup_stats_update(quint32)));

	mPortGroups.append(&portGroup);
	portGroup.connectToHost();

	mPortGroupListModel.portGroupAppended();

	mPortStatsModel.when_portListChanged();
}

void PortGroupList::removePortGroup(PortGroup &portGroup)
{
	mPortGroupListModel.portGroupAboutToBeRemoved(&portGroup);

	PortGroup* pg = mPortGroups.takeAt(mPortGroups.indexOf(&portGroup));
	qDebug("after takeAt()");
	mPortGroupListModel.portGroupRemoved();

	delete pg;

	mPortStatsModel.when_portListChanged();
}

//....................
// Private Methods
//....................
int PortGroupList::indexOfPortGroup(quint32 portGroupId)
{
	for (int i = 0; i < mPortGroups.size(); i++) {
		if (mPortGroups.value(i)->id() == portGroupId)
			return i;
	} 
	return -1;
}
