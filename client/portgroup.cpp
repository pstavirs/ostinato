#include "portgroup.h"
#include "../common/protocol.h"

quint32	PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QHostAddress ip, quint16 port)
{
	// Allocate an id for self
	mPortGroupId = PortGroup::mPortGroupAllocId++;

	// Init attributes for which we values were passed to us
	mServerAddress = ip;
	mServerPort = port;

	// Init remaining attributes with defaults
	mpSocket = new QTcpSocket(this);

	// TODO: consider using QT's signal-slot autoconnect
	connect(mpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(on_mpSocket_stateChanged()));
	connect(mpSocket, SIGNAL(connected()), this, SLOT(when_connected()));
	connect(mpSocket, SIGNAL(disconnected()), this, SLOT(when_disconnected()));
	connect(mpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(when_error(QAbstractSocket::SocketError)));
	connect(mpSocket, SIGNAL(readyRead()), this, SLOT(when_dataAvail()));
}

PortGroup::~PortGroup()
{
	qDebug("PortGroup Destructor");
	// Disconnect and free TCP mpSocketet etc.
	PortGroup::disconnectFromHost();
	delete mpSocket;
}

void PortGroup::connectToHost(QHostAddress ip, quint16 port)
{
	mServerAddress = ip;
	mServerPort = port;

	PortGroup::connectToHost();
}

void PortGroup::connectToHost()
{
	qDebug("PortGroup::connectToHost()");
	mpSocket->connectToHost(mServerAddress, mServerPort);
}

void PortGroup::disconnectFromHost()
{
	mpSocket->disconnectFromHost();
}

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

		p = new Port(NTOHL(cap->port), mPortGroupId);
		p->setName(cap->name);
		p->setDescription(cap->desc);
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

// ------------------------------------------------
//                      Slots
// ------------------------------------------------
void PortGroup::on_mpSocket_stateChanged()
{
	qDebug("state changed");
	emit portGroupDataChanged(this);
}

void PortGroup::when_connected()
{
	qDebug("connected\n");
	
	emit portGroupDataChanged(this);

	// Ask for Port Capability
	tCommHdr	pkt;
	pkt.ver = 1;
	pkt.resv1 = 0;
	pkt.resv2 = 0;
	pkt.msgType = HTONS(e_MT_CapabilityReq);
	pkt.msgLen = HTONS(8);

	mpSocket->write((char*) &pkt, sizeof(pkt));
}

void PortGroup::when_disconnected()
{
	qDebug("disconnected\n");
	emit portListAboutToBeChanged(mPortGroupId);
	mPorts.clear();
	emit portListChanged(mPortGroupId);
	emit portGroupDataChanged(this);
}

void PortGroup::when_error(QAbstractSocket::SocketError socketError)
{
	qDebug("error\n");
	emit portGroupDataChanged(this);
}

void PortGroup::when_dataAvail()
{
	qDebug("dataAvail\n");
	
	QByteArray	msg = mpSocket->read(1024);	// FIXME: hardcoding
	ProcessMsg(msg.constData(), msg.size());
}


