#include "portgroup.h"
#include "../common/protocol.h"

#include <vector>

quint32	PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QHostAddress ip, quint16 port)
{
	// Allocate an id for self
	mPortGroupId = PortGroup::mPortGroupAllocId++;

#if 0 // PB
	// Init attributes for which we values were passed to us
	mServerAddress = ip;
	mServerPort = port;

	// Init remaining attributes with defaults
	mpSocket = new QTcpSocket(this);
#endif 

	rpcChannel = new PbRpcChannel(ip, port);
	rpcController = new PbRpcController();
	serviceStub = new OstProto::OstService::Stub(rpcChannel,
		OstProto::OstService::STUB_OWNS_CHANNEL);

#if 0 // PB
	// TODO: consider using QT's signal-slot autoconnect
	connect(mpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(on_mpSocket_stateChanged()));
	connect(mpSocket, SIGNAL(connected()), this, SLOT(when_connected()));
	connect(mpSocket, SIGNAL(disconnected()), this, SLOT(when_disconnected()));
	connect(mpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(when_error(QAbstractSocket::SocketError)));
	connect(mpSocket, SIGNAL(readyRead()), this, SLOT(when_dataAvail()));
#endif
	// FIXME:Can't for my life figure out why this ain't working!
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

#if 0 // PB
void PortGroup::connectToHost(QHostAddress ip, quint16 port)
{
	rpcChannel->establish(ip, port)
}

void PortGroup::connectToHost()
{
	qDebug("PortGroup::connectToHost()");
	rpcChannel->establish()
}

void PortGroup::disconnectFromHost()
{
	mpSocket->disconnectFromHost();
}
#endif

#if 0 // PB
// --------------------------------------------
// Private Methods 
// --------------------------------------------
void PortGroup::ProcessMsg(const char *msg, quint32 size)
{
	tCommHdr *hdr;
	// TODO: For now, assuming we'll get a complete msg
	// but need to fix this as this is a TCP stream

	hdr = (tCommHdr*) msg;

	if (hdr->ver != 1) // FIXME:hardcoding
	{
		qDebug("Rcvd msg with invalid version %d\n", hdr->ver);
		goto _exit;
	}

	qDebug("msgType - %x\n", NTOHS(hdr->msgType));
	switch (NTOHS(hdr->msgType))
	{
		case e_MT_CapabilityInfo:
			ProcessCapabilityInfo(msg+sizeof(tCommHdr), 
				NTOHS(hdr->msgLen)-sizeof(tCommHdr));
			break;

		default:
			qDebug("Rcvd msg with unrecognized msgType %d\n", NTOHS(hdr->msgType));
	}
	
_exit:
	return;
}

void PortGroup::ProcessCapabilityInfo(const char *msg, qint32 size)
{
	tTlvPortCapability	*cap = (tTlvPortCapability*) msg;
	Port *p;

	emit portListAboutToBeChanged(mPortGroupId);

	while (size)
	{
		qDebug("size = %d, tlvType = %d, tlvLen = %d\n",
			size, NTOHS(cap->tlvType), NTOHS(cap->tlvLen));
		if (NTOHS(cap->tlvType) != e_TT_PortCapability)
		{
			qDebug("Unrecognized TLV Type %d\n", NTOHS(cap->tlvType));
			goto _next;
		}

		p = new Port(NTOHL(cap->portId), mPortGroupId);
		p->setName(cap->portName);
		p->setDescription(cap->portDesc);
		p->insertDummyStreams(); // FIXME: only for testing
		qDebug("before port append\n");
		mPorts.append(*p);

_next:
		size -= NTOHS(cap->tlvLen);
		cap = (tTlvPortCapability*)((char *)(cap) + NTOHS(cap->tlvLen));
	}

	emit portListChanged(mPortGroupId);

	return;
}
#endif

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

#if 0 // PB
	// Ask for Port Capability
	tCommHdr	pkt;
	pkt.ver = 1;
	pkt.resv1 = 0;
	pkt.resv2 = 0;
	pkt.msgType = HTONS(e_MT_GetCapability);
	pkt.msgLen = HTONS(8);

	mpSocket->write((char*) &pkt, sizeof(pkt));
#endif
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

#if 0 // PB
void PortGroup::when_dataAvail()
{
	qDebug("dataAvail\n");
	
	QByteArray	msg = mpSocket->read(1024);	// FIXME: hardcoding
	ProcessMsg(msg.constData(), msg.size());
}
#endif

void PortGroup::processPortIdList(OstProto::PortIdList *portIdList)
{
	int count;

	qDebug("got a portlist ...");

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _error_exit;
	}

	count = portIdList->port_id_size();
	qDebug("%s: portid count = %d", __FUNCTION__, count);
	qDebug("%s: %s", __FUNCTION__, portIdList->DebugString().c_str());

	emit portListAboutToBeChanged(mPortGroupId);

	for(int i = 0; i < count; i++)
	{
		Port *p;
		
		p = new Port(portIdList->port_id(i), mPortGroupId);
		//p->setName("name");
		//p->setDescription("Desc");
		p->insertDummyStreams(); // FIXME: only for testing
		qDebug("before port append\n");
		mPorts.append(*p);
	}

	emit portListChanged(mPortGroupId);

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
	int count;

	qDebug("In %s", __FUNCTION__);

	if (rpcController->Failed())
	{
		qDebug("%s: rpc failed", __FUNCTION__);
		goto _error_exit;
	}

	count = portConfigList->list_size();
	qDebug("%s: count = %d", __FUNCTION__, count);
	qDebug("%s: <%s>", __FUNCTION__, portConfigList->DebugString().c_str());

	emit portListAboutToBeChanged(mPortGroupId);

	for(int i = 0; i < count; i++)
	{
		uint	id;

		id = portConfigList->list(i).port_id();
		// FIXME: don't mix port id & index into mPorts[]
		mPorts[id].updatePortConfig(portConfigList->mutable_list(i));
	}

	emit portListChanged(mPortGroupId);

	// FIXME: check if we need new signals since we are not changing the
	// number of ports, just the port data

_error_exit:
	delete portConfigList;
}
