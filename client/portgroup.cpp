#include "portgroup.h"
#include "../common/protocol.h"

#include <vector>

quint32	PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QHostAddress ip, quint16 port)
{
	// Allocate an id for self
	mPortGroupId = PortGroup::mPortGroupAllocId++;

	rpcChannel = new PbRpcChannel(ip, port);
	rpcController = new PbRpcController();
	serviceStub = new OstProto::OstService::Stub(rpcChannel,
		OstProto::OstService::STUB_OWNS_CHANNEL);

	// FIXME(LOW):Can't for my life figure out why this ain't working!
	//QMetaObject::connectSlotsByName(this);
	connect(rpcChannel, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
		this, SLOT(on_rpcChannel_stateChanged()));
	connect(rpcChannel, SIGNAL(connected()), 
		this, SLOT(on_rpcChannel_connected()));
	connect(rpcChannel, SIGNAL(disconnected()),
		this, SLOT(on_rpcChannel_disconnected()));
	connect(rpcChannel, SIGNAL(error(QAbstractSocket::SocketError)), 
		this, SLOT(on_rpcChannel_error(QAbstractSocket::SocketError)));
}

PortGroup::~PortGroup()
{
	qDebug("PortGroup Destructor");
	// Disconnect and free rpc channel etc.
	PortGroup::disconnectFromHost();
	delete serviceStub;
}


// ------------------------------------------------
//                      Slots
// ------------------------------------------------
void PortGroup::on_rpcChannel_stateChanged()
{
	qDebug("state changed");
	emit portGroupDataChanged(this);
}

void PortGroup::on_rpcChannel_connected()
{
	OstProto::Void			void_;
	OstProto::PortIdList	*portIdList;
	
	qDebug("connected\n");
	emit portGroupDataChanged(this);

	qDebug("requesting portlist ...");
	portIdList = new OstProto::PortIdList();
	rpcController->Reset();
	serviceStub->getPortIdList(rpcController, &void_, portIdList, 
		NewCallback(this, &PortGroup::processPortIdList, portIdList));
}

void PortGroup::on_rpcChannel_disconnected()
{
	qDebug("disconnected\n");
	emit portListAboutToBeChanged(mPortGroupId);
	mPorts.clear();
	emit portListChanged(mPortGroupId);
	emit portGroupDataChanged(this);
}

void PortGroup::on_rpcChannel_error(QAbstractSocket::SocketError socketError)
{
	qDebug("error\n");
	emit portGroupDataChanged(this);
}

void PortGroup::when_configApply(int portIndex, uint *cookie)
{
	uint			*op;
	OstProto::Ack	*ack;

	Q_ASSERT(portIndex < mPorts.size());

	if (cookie == NULL)
	{
		// cookie[0]: op [0 - delete, 1 - add, 2 - modify, 3 - Done!]
		// cookie[1]: *ack
		cookie = new uint[2];
		ack = new OstProto::Ack;

		cookie[0] = (uint) 0;
		cookie[1] = (uint) ack;
	}
	else
	{
		ack = (OstProto::Ack*) cookie[1];
	}

	Q_ASSERT(cookie != NULL);
	op = &cookie[0];

	switch (*op)
	{
	case 0:
		{
			OstProto::StreamIdList		streamIdList;

			qDebug("applying 'deleted streams' ...");

			streamIdList.mutable_port_id()->set_id(mPorts[portIndex].id());
			mPorts[portIndex].getDeletedStreamsSinceLastSync(streamIdList);

			(*op)++;
			rpcController->Reset();
			serviceStub->deleteStream(rpcController, &streamIdList, ack, 
				::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
			break;
		}

	case 1:
		{
			OstProto::StreamIdList		streamIdList;

			qDebug("applying 'new streams' ...");

			streamIdList.mutable_port_id()->set_id(mPorts[portIndex].id());
			mPorts[portIndex].getNewStreamsSinceLastSync(streamIdList);

			(*op)++;
			rpcController->Reset();
			serviceStub->addStream(rpcController, &streamIdList, ack, 
				::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
			break;
		}

	case 2:
		{
			OstProto::StreamConfigList	streamConfigList;

			qDebug("applying 'modified streams' ...");

			streamConfigList.mutable_port_id()->set_id(mPorts[portIndex].id());
			mPorts[portIndex].getModifiedStreamsSinceLastSync(streamConfigList);

			(*op)++;
			rpcController->Reset();
			serviceStub->modifyStream(rpcController, &streamConfigList, ack, 
				::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
			break;
		}

	case 3:
		qDebug("apply completed");
		delete cookie;
		break;

	default:
		qDebug("%s: Unknown Op!!!", __FUNCTION__);
		break;
	}
}

void PortGroup::processPortIdList(OstProto::PortIdList *portIdList)
{
	qDebug("got a portlist ...");

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _error_exit;
	}

	emit portListAboutToBeChanged(mPortGroupId);

	for(int i = 0; i <  portIdList->port_id_size(); i++)
	{
		Port *p;
		
		p = new Port(portIdList->port_id(i).id(), mPortGroupId);
		qDebug("before port append\n");
		mPorts.append(*p);
	}

	emit portListChanged(mPortGroupId);

	this->portIdList.CopyFrom(*portIdList);

	// Request PortConfigList
	{
		OstProto::PortConfigList	*portConfigList;
		
		qDebug("requesting port config list ...");
		portConfigList = new OstProto::PortConfigList();
		rpcController->Reset();
		serviceStub->getPortConfig(rpcController,
			portIdList, portConfigList, NewCallback(this, 
				&PortGroup::processPortConfigList, portConfigList));
	}

	goto _exit;

_error_exit:
_exit:
	delete portIdList;
}

void PortGroup::processPortConfigList(OstProto::PortConfigList *portConfigList)
{
	qDebug("In %s", __FUNCTION__);

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _error_exit;
	}

	emit portListAboutToBeChanged(mPortGroupId);

	for(int i = 0; i < portConfigList->port_size(); i++)
	{
		uint	id;

		id = portConfigList->port(i).port_id().id();
		// FIXME: don't mix port id & index into mPorts[]
		mPorts[id].updatePortConfig(portConfigList->mutable_port(i));
	}

	emit portListChanged(mPortGroupId);

	// FIXME: check if we need new signals since we are not changing the
	// number of ports, just the port data

	if (numPorts() > 0)
		getStreamIdList();

_error_exit:
	delete portConfigList;
}

void PortGroup::getStreamIdList(int portIndex, 
	OstProto::StreamIdList *streamIdList)
{
	::OstProto::PortId			portId;

	qDebug("In %s", __FUNCTION__);

	if (streamIdList == NULL)
	{
		// First invocation (uses default params) - 
		//     request StreamIdList for first port

		Q_ASSERT(portIndex == 0);
		Q_ASSERT(numPorts() > 0);
		streamIdList = new ::OstProto::StreamIdList();

		goto _request;
	}

	qDebug("got a streamIdlist ...");

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _next_port; 	// FIXME(MED): Partial RPC
	}

	Q_ASSERT(portIndex < numPorts());

	if (streamIdList->port_id().id() != mPorts[portIndex].id())
	{
		qDebug("%s: Invalid portId %d (expected %d) received for portIndex %d",
			__FUNCTION__, streamIdList->port_id().id(), mPorts[portIndex].id(), 
			portIndex);
		goto _next_port; 	// FIXME(MED): Partial RPC
	}

	// FIXME(MED): need to mPorts.clear()???
	for(int i = 0; i < streamIdList->stream_id_size(); i++)
	{
		uint streamId;

		streamId = streamIdList->stream_id(i).id();
		mPorts[portIndex].insertStream(streamId);
	}

_next_port:
	// FIXME(HI): ideally we shd use signals/slots but this means
	// we will have to use Port* instead of Port with QList<> -
	// need to find a way for this
	mPorts[portIndex].when_syncComplete();
	portIndex++;
	if (portIndex >= numPorts())
	{
		// We're done for all ports !!!

		// FIXME(HI): some way to reset streammodel 

		delete streamIdList;

		if (numPorts() > 0)
			getStreamConfigList();

		goto _exit;
	}

_request:
	portId.set_id(mPorts[portIndex].id());
	streamIdList->Clear();

	rpcController->Reset();
	serviceStub->getStreamIdList(rpcController, &portId, streamIdList,
		NewCallback(this, &PortGroup::getStreamIdList,
			portIndex, streamIdList));

	goto _exit;



_exit:
	return;
}

void PortGroup::getStreamConfigList(int portIndex,
	OstProto::StreamConfigList *streamConfigList)
{
	OstProto::StreamIdList	streamIdList;

	qDebug("In %s", __PRETTY_FUNCTION__);

	if (streamConfigList == NULL)
	{
		// First invocation using default params 
		//		- request for first port

		Q_ASSERT(portIndex == 0);
		Q_ASSERT(numPorts() > 0);

		streamConfigList = new OstProto::StreamConfigList;

		goto _request;
	}

	qDebug("got a streamconfiglist");

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _next_port;
	}

	Q_ASSERT(portIndex < numPorts());

	if (streamConfigList->port_id().id() != mPorts[portIndex].id())
	{
		qDebug("%s: Invalid portId %d (expected %d) received for portIndex %d",
			__FUNCTION__, streamConfigList->port_id().id(), 
			mPorts[portIndex].id(), portIndex);
		goto _next_port; 	// FIXME(MED): Partial RPC
	}

	// FIXME(MED): need to mStreams.clear()???
	for(int i = 0; i <  streamConfigList->stream_size(); i++)
	{
		uint streamId;

		streamId = streamConfigList->stream(i).stream_id().id();
		mPorts[portIndex].updateStream(streamId,
			streamConfigList->mutable_stream(i));
	}

_next_port:
	portIndex++;

	if (portIndex >= numPorts())
	{
		// We're done for all ports !!!

		// FIXME(HI): some way to reset streammodel 

		delete streamConfigList;
		goto _exit;
	}

_request:
	qDebug("requesting stream config list ...");

	streamIdList.Clear();
	streamIdList.mutable_port_id()->set_id(mPorts[portIndex].id());
	for (int j = 0; j < mPorts[portIndex].numStreams(); j++)
	{
		OstProto::StreamId	*s;

		s = streamIdList.add_stream_id();
		s->set_id(mPorts[portIndex].streamByIndex(j).id());
	}
	streamConfigList->Clear();

	rpcController->Reset();
	serviceStub->getStreamConfig(rpcController,
		&streamIdList, streamConfigList, NewCallback(this, 
			&PortGroup::getStreamConfigList, portIndex, streamConfigList));

_exit:
	return;
}

void PortGroup::processModifyStreamAck(OstProto::Ack *ack)
{
	qDebug("In %s", __FUNCTION__);

	qDebug("Modify Successful!!");

	// TODO(HI): Apply Button should now be disabled???!!!!???
}

void PortGroup::startTx(QList<uint> portList)
{
	OstProto::PortIdList	portIdList;
	OstProto::Ack			*ack = new OstProto::Ack;

	qDebug("In %s", __FUNCTION__);

	for (int i = 0; i < portList.size(); i++)
	{
		OstProto::PortId	*portId;
		portId = portIdList.add_port_id();
		portId->set_id(portList.at(i));
	}

	serviceStub->startTx(rpcController, &portIdList, ack,
			NewCallback(this, &PortGroup::processStartTxAck, ack));
}

void PortGroup::processStartTxAck(OstProto::Ack	*ack)
{
	qDebug("In %s", __FUNCTION__);

	delete ack;
}

void PortGroup::getPortStats()
{
	OstProto::PortStatsList	*portStatsList = new OstProto::PortStatsList;

	qDebug("In %s", __FUNCTION__);

	serviceStub->getStats(rpcController, &portIdList, portStatsList,
			NewCallback(this, &PortGroup::processPortStatsList, portStatsList));
}

void PortGroup::processPortStatsList(OstProto::PortStatsList *portStatsList)
{
	qDebug("In %s", __FUNCTION__);

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _error_exit;
	}

	for(int i = 0; i < portStatsList->port_stats_size(); i++)
	{
		uint	id;

		id = portStatsList->port_stats(i).port_id().id();
		// FIXME: don't mix port id & index into mPorts[]
		mPorts[id].updateStats(portStatsList->mutable_port_stats(i));
	}

	emit statsChanged(mPortGroupId);

_error_exit:
	delete portStatsList;
}

void PortGroup::clearPortStats()
{
	OstProto::Ack	*ack = new OstProto::Ack;

	qDebug("In %s", __FUNCTION__);

	serviceStub->clearStats(rpcController, &portIdList, ack,
			NewCallback(this, &PortGroup::processClearStatsAck, ack));
}

void PortGroup::processClearStatsAck(OstProto::Ack	*ack)
{
	qDebug("In %s", __FUNCTION__);

	delete ack;
}

