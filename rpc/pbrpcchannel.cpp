#include "pbrpcchannel.h"

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
	char	msg[MSGBUF_SIZE];
	int		len;
	bool	ret;
  
	if (isPending)
	{
		RpcCall call;
		qDebug("RpcChannel: queueing method %d since %d is pending", 
			method->index(), pendingMethodId);

		call.method = method;
		call.controller = controller;
		call.request = req;
		call.response = response;
		call.done = done;

		pendingCallList.append(call);

		Q_ASSERT(pendingCallList.size() < 100);

		return;
	}

	if (!req->IsInitialized())
	{
		qWarning("RpcChannel: missing required fields in request");
		qDebug(req->InitializationErrorString().c_str());

		qFatal("exiting");

		controller->SetFailed("Required fields missing");
		done->Run();
		return;
	}

	pendingMethodId = method->index();
	this->controller=controller;
	this->done=done;
	this->response=response;
	isPending = true;

	ret = req->SerializeToArray((void*) (&msg[PB_HDR_SIZE]), sizeof(msg));
	Q_ASSERT(ret == true);

	len = req->ByteSize();
	*((quint16*)(&msg[0])) = HTONS(PB_MSG_TYPE_REQUEST); // type
	*((quint16*)(&msg[2])) = HTONS(method->index()); // method id
	*((quint32*)(&msg[4])) = HTONL(len); // len

	// Avoid printing stats since it happens every couple of seconds
	if (pendingMethodId != 12)
	{
		qDebug("client(%s) sending %d bytes encoding <%s>", __FUNCTION__, 
				PB_HDR_SIZE + len, req->DebugString().c_str());
		BUFDUMP(msg, PB_HDR_SIZE + len);
	}

	mpSocket->write(msg, PB_HDR_SIZE + len);
}

void PbRpcChannel::on_mpSocket_readyRead()
{
	char	msg[MSGBUF_SIZE];
	char	*p = (char*)&msg;
	int		msgLen;
	static bool parsing = false;
	static quint16	type, method;
	static quint32	len;

	//qDebug("%s: bytesAvail = %d", __FUNCTION__, mpSocket->bytesAvailable());

	if (!parsing)
	{
		// Do we have an entire header? If not, we'll wait ...
		if (mpSocket->bytesAvailable() < PB_HDR_SIZE)
		{
			qDebug("client: not enough data available for a complete header");
			return;
		}

		msgLen = mpSocket->read(msg, PB_HDR_SIZE);

		Q_ASSERT(msgLen == PB_HDR_SIZE);

		type = NTOHS(GET16(p+0));
		method = NTOHS(GET16(p+2));
		len = NTOHL(GET32(p+4));

		//BUFDUMP(msg, PB_HDR_SIZE);
		//qDebug("type = %hu, method = %hu, len = %u", type, method, len);

		parsing = true;
	}

	switch (type)
	{
		case PB_MSG_TYPE_BINBLOB:
		{
			static quint32 cumLen = 0;
			QIODevice *blob;

			blob = static_cast<PbRpcController*>(controller)->binaryBlob();
			Q_ASSERT(blob != NULL);

			while ((cumLen < len) && mpSocket->bytesAvailable())
			{
				int l;

				l = mpSocket->read(msg, sizeof(msg));
				blob->write(msg, l);
				cumLen += l;
			}

			qDebug("%s: bin blob rcvd %d/%d", __PRETTY_FUNCTION__, cumLen, len);

			if (cumLen < len)
				return;

			cumLen = 0;

			if (!isPending)
			{
				qDebug("not waiting for response");
				goto _error_exit;
			}

			if (pendingMethodId != method)
			{
				qDebug("invalid method id %d (expected = %d)", method, 
					pendingMethodId);
				goto _error_exit;
			}

			break;
		}

		case PB_MSG_TYPE_RESPONSE:
			// Wait till we have the entire message
			if (mpSocket->bytesAvailable() < len)
			{
				qDebug("client: not enough data available for a complete msg");
				return;
			}
			
			msgLen = mpSocket->read(msg, sizeof(msg));

			Q_ASSERT((unsigned) msgLen == len);

			//qDebug("client(%s) rcvd %d bytes", __FUNCTION__, msgLen);
			//BUFDUMP(msg, msgLen);

			if (!isPending)
			{
				qDebug("not waiting for response");
				goto _error_exit;
			}

			if (pendingMethodId != method)
			{
				qDebug("invalid method id %d (expected = %d)", method, 
					pendingMethodId);
				goto _error_exit;
			}

			response->ParseFromArray((void*) msg, len);

			// Avoid printing stats
			if (method != 12)
			{
				qDebug("client(%s): Parsed as %s", __FUNCTION__,
					response->DebugString().c_str());
			}

			if (!response->IsInitialized())
			{
				qWarning("RpcChannel: missing required fields in response");
				qDebug(response->InitializationErrorString().c_str());

				controller->SetFailed("Required fields missing");
			}
			break;

		default:
			qFatal("%s: unexpected type %d", __PRETTY_FUNCTION__, type);
			goto _error_exit;
				
	}

	pendingMethodId = -1;
	controller = NULL;
	response = NULL;
	isPending = false;
	parsing = false;

	done->Run();

	if (pendingCallList.size())
	{
		RpcCall call = pendingCallList.takeFirst();
		CallMethod(call.method, call.controller, call.request, call.response,
				call.done);
	}

	return;

_error_exit:
	parsing = false;
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

	pendingMethodId = -1;
	controller = NULL;
	response = NULL;
	isPending = false;
	// \todo convert parsing from static to data member
	//parsing = false 
	pendingCallList.clear();

	emit disconnected();
}

void PbRpcChannel::on_mpSocket_error(QAbstractSocket::SocketError socketError)
{
	qDebug("In %s", __FUNCTION__);
	emit error(socketError);
}

