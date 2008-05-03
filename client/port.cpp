#include "port.h"

Port::Port(quint32 id, quint32 portGroupId)
{
	mPortId = id;
	mPortGroupId = portGroupId;

	mAdminStatus = AdminDisable;
	mOperStatus = OperDown;
	mControlMode = ControlShared;

	// FIXME(HI): TEST only
	for(int i = 0; i < 10; i++)
		mPortStats[i] = mPortGroupId*10000+mPortId*100+i;
}

void Port::insertDummyStreams()
{
	mStreams.append(*(new Stream));
	mStreams[0].setName(QString("%1:%2:0").arg(portGroupId()).arg(id()));
	mStreams.append(*(new Stream));
	mStreams[1].setName(QString("%1:%2:1").arg(portGroupId()).arg(id()));
}

