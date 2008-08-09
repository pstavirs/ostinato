#include "pbrpcchannel.h"

//#include "../common/protocol.pb.h"

PbRpcChannel::PbRpcChannel(QHostAddress ip, quint16 port)
{
	isPending = false;
	pendingMethodId = -1;	// don't care as long as isPending is false

	controller = NULL;
	done = NULL;
	response = NULL;

	mServerAddress = ip;
	mServerPort = port;
	mpSocket = new QTcpSocket(this);

	// FIXME: Not quite sure why this ain't working!
	// QMetaObject::connectSlotsByName(this);

	connect(mpSocket, SIGNAL(connected()),
		this, SLOT(on_mpSocket_connected()));
	connect(mpSocket, SIGNAL(disconnected()),
		this, SLOT(on_mpSocket_disconnected()));
	connect(mpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
		this, SLOT(on_mpSocket_stateChanged(QAbstractSocket::SocketState)));
	connect(mpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
		this, SLOT(on_mpSocket_error(QAbstractSocket::SocketError)));

	connect(mpSocket, SIGNAL(readyRead()),
		this, SLOT(on_mpSocket_readyRead()));

}

PbRpcChannel::~PbRpcChannel()
{
	delete mpSocket;
}

void PbRpcChannel::establish()
{
	qDebug("In %s", __FUNCTION__);

	mpSocket->connectToHost(mServerAddress, mServerPort);
}

void PbRpcChannel::establish(QHostAddress ip, quint16 port)
{
	mServerAddress = ip;
	mServerPort = port;
	establish();
}

void PbRpcChannel::tearDown()
{
	qDebug("In %s", __FUNCTION__);

	mpSocket->disconnectFromHost();
}

void PbRpcChannel::CallMethod(
	const ::google::protobuf::MethodDescriptor *method,
	::google::protobuf::RpcController *controller,
	const ::google::protobuf::Message *req,
	::google::protobuf::Message *response,
	::google::protobuf::Closure* done)
{
	char	msg[1024];	// FIXME: hardcoding
	char 	*p = (char *)&msg;
	int		len;
  
	qDebug("In %s", __FUNCTION__);

	pendingMethodId = method->index();
	this->controller=controller;
	this->done=done;
	this->response=response;
	isPending = true;

	*((quint16*)(p+0)) = HTONS(PB_MSG_TYPE_REQUEST); // type
	qDebug("CLi:GET16 = %d/%d, type = %d", GET16(p+0), NTOHS(GET16(p+0)), 
		PB_MSG_TYPE_REQUEST);
	*((quint16*)(p+2)) = HTONS(method->index()); // method id
	// (p+4) len later after serialization
	*((quint16*)(p+6)) = HTONS(0); // rsvd

	// SerialData is at offset 8
	req->SerializeToArray((void*) (p+8), sizeof(msg));

	len = req->ByteSize();
	*((quint16*)(p+4)) = HTONS(len); // len

	qDebug("client(%s) sending %d bytes encoding <%s>", __FUNCTION__, len+8,
		req->ShortDebugString().c_str());
	BUFDUMP(msg, len+8);

	mpSocket->write(msg, len + 8);
}

void PbRpcChannel::on_mpSocket_readyRead()
{
	char	msg[1024]; // FIXME: hardcoding;
	char	*p = (char*)&msg;
	int		msgLen;
	quint16	type, method, len, rsvd;
	PbRpcController	*controller;

	qDebug("In %s", __FUNCTION__);
	
	msgLen = mpSocket->read(msg, sizeof(msg));

	qDebug("client(%s) rcvd %d bytes", __FUNCTION__, msgLen);
	BUFDUMP(msg, msgLen);

	type = NTOHS(GET16(p+0));
	method = NTOHS(GET16(p+2));
	len = NTOHS(GET16(p+4));
	rsvd = NTOHS(GET16(p+6));

	if (!isPending)
	{
		qDebug("not waiting for response");
		
		goto _error_exit;
	}

	if (type != PB_MSG_TYPE_RESPONSE)
	{
		qDebug("invalid msgType %d (expected = %d)", type, 
			PB_MSG_TYPE_RESPONSE);
		
		goto _error_exit;
	}

	if (pendingMethodId != method)
	{
		qDebug("invalid method id %d (expected = %d)", method, 
			pendingMethodId);
		
		goto _error_exit;
	}


	// Serialized data starts from offset 8
	response->ParseFromArray((void*) &msg[8], len);
	qDebug("client(%s): Parsed as %s", __FUNCTION__,
		response->DebugString().c_str());


	pendingMethodId = -1;
	controller = NULL;
	//done = NULL;
	response = NULL;
	isPending = false;

	done->Run();

	return;

_error_exit:
	qDebug("client(%s) discarding received msg", __FUNCTION__);
	return;
}

void PbRpcChannel::on_mpSocket_stateChanged(
	QAbstractSocket::SocketState socketState)
{
	qDebug("In %s", __FUNCTION__);
	emit stateChanged(socketState);
}

void PbRpcChannel::on_mpSocket_connected()
{
	qDebug("In %s", __FUNCTION__);
	emit connected(); 
}

void PbRpcChannel::on_mpSocket_disconnected()
{
	qDebug("In %s", __FUNCTION__);
	emit disconnected();
}

void PbRpcChannel::on_mpSocket_error(QAbstractSocket::SocketError socketError)
{
	qDebug("In %s", __FUNCTION__);
	emit error(socketError);
}

