#include "pbhelper.h"
#include "rpcserver.h"

RpcServer::RpcServer()
{
	server = NULL;
	clientSock = NULL;

	service = NULL; 

	isPending = false;
	pendingMethodId = -1; // don't care as long as isPending is false
}

RpcServer::~RpcServer()
{ 
	if (server) 
		delete server; 
}

bool RpcServer::registerService(::google::protobuf::Service *service, 
	quint16 tcpPortNum)
{
	this->service = service;

	server = new QTcpServer();
	connect(server, SIGNAL(newConnection()), this, SLOT(when_newConnection()));
	if (!server->listen(QHostAddress::Any, tcpPortNum)) 
	{
		LogInt(tr("Unable to start the server: %1").arg(server->errorString()));
		return false;
	}

	LogInt(tr("The server is running on %1:%2").arg(server->serverAddress().toString()).arg(server->serverPort()));
	return true;
}

void RpcServer::done(::google::protobuf::Message *resp, PbRpcController *PbRpcController)
{
	char	msg[4096];	// FIXME: hardcoding
	char 	*p = (char *)&msg;
	int		len;

	qDebug("In RpcServer::done");

	// TODO: check PbRpcController to see if method failed
	if (PbRpcController->Failed())
	{
		qDebug("rpc failed");
		goto _exit;
	}

	if (!resp->IsInitialized())
	{
		qDebug("response missing required fields!!");
		qDebug(resp->InitializationErrorString().c_str());
		goto _exit;
	}

	*((quint16*)(p+0)) = HTONS(PB_MSG_TYPE_RESPONSE); // type TODO:RESPONSE
	*((quint16*)(p+2)) = HTONS(pendingMethodId); // method
	*((quint16*)(p+6)) = HTONS(0); // rsvd

	// SerialData is at offset 8
	resp->SerializeToArray((void*) (p+8), sizeof(msg));

	len = resp->ByteSize();
	(*(quint16*)(p+4)) = HTONS(len); // len

	qDebug("Server(%s): sending %d bytes to client encoding <%s>", 
		__FUNCTION__, len + 8, resp->DebugString().c_str());
	//BUFDUMP(msg, len + 8);

	clientSock->write(msg, len + 8);

_exit:
	delete PbRpcController;
	isPending = false;
}

void RpcServer::when_newConnection()
{
	if (clientSock)
	{
		QTcpSocket	*sock;

		LogInt(tr("already connected, no new connections will be accepted\n"));

		// Accept and close connection
		// TODO: Send reason msg to client
		sock = server->nextPendingConnection();
		sock->disconnectFromHost();
		sock->deleteLater();
		goto _exit;
	}

	clientSock = server->nextPendingConnection();
	LogInt(tr("accepting new connection from %1:%2").arg(clientSock->peerAddress().toString()).arg(clientSock->peerPort()));

	connect(clientSock, SIGNAL(readyRead()), 
		this, SLOT(when_dataAvail()));
	connect(clientSock, SIGNAL(disconnected()), 
		this, SLOT(when_disconnected()));
	connect(clientSock, SIGNAL(error(QAbstractSocket::SocketError)), 
		this, SLOT(when_error(QAbstractSocket::SocketError)));

_exit:
	return;
}

void RpcServer::when_disconnected()
{
	LogInt(tr("connection closed from %1:%2").arg(clientSock->peerAddress().toString()).arg(clientSock->peerPort()));

	clientSock->deleteLater();
	clientSock = NULL;
}

void RpcServer::when_error(QAbstractSocket::SocketError socketError)
{
	LogInt(clientSock->errorString());
}

void RpcServer::when_dataAvail()
{
	char	msg[4096]; // FIXME: hardcoding;
	int		msgLen;
	char	*p = (char*) &msg;
	quint16	type, method, len, rsvd;
	const ::google::protobuf::MethodDescriptor	*methodDesc;
	::google::protobuf::Message	*req, *resp;
	PbRpcController	*controller;
	
	msgLen = clientSock->read(msg, sizeof(msg));
	LogInt(QString(QByteArray(msg, msgLen).toHex()));

	qDebug("Server %s: rcvd %d bytes", __FUNCTION__, msgLen);
	//BUFDUMP(msg, msgLen);

	type = NTOHS(GET16(p+0));
	method = NTOHS(GET16(p+2));
	len = NTOHS(GET16(p+4));
	rsvd = NTOHS(GET16(p+6));
	qDebug("type = %d, method = %d, len = %d, rsvd = %d", 
		type, method, len, rsvd);

	if (type != PB_MSG_TYPE_REQUEST)
	{
		qDebug("server(%s): unexpected msg type %d (expected %d)", __FUNCTION__,
			type, PB_MSG_TYPE_REQUEST);
		goto _error_exit;
	}
	

	methodDesc = service->GetDescriptor()->method(method);
	if (!methodDesc)
	{
		qDebug("server(%s): invalid method id %d", __FUNCTION__, method);
		goto _error_exit; // TODO: Return Error to client
	}

	if (isPending)
	{
		qDebug("server(%s): rpc pending, try again", __FUNCTION__);
		goto _error_exit; // TODO: Return Error to client
	}

	pendingMethodId = method;
	isPending = true;

	req = service->GetRequestPrototype(methodDesc).New();
	resp = service->GetResponsePrototype(methodDesc).New();

	// Serialized data starts from offset 8
	req->ParseFromArray((void*) (msg+8), len);
	if (!req->IsInitialized())
	{
		qDebug("Missing required fields in request");
		qDebug(req->InitializationErrorString().c_str());
		delete req;
		delete resp;

		goto _error_exit;
	}
	qDebug("Server(%s): successfully parsed as <%s>", __FUNCTION__, 
		resp->DebugString().c_str());

	controller = new PbRpcController;

	qDebug("before service->callmethod()");

	service->CallMethod(methodDesc, controller, req, resp,
		NewCallback(this, &RpcServer::done, resp, controller));

	return;

_error_exit:
	qDebug("server(%s): discarding msg from client", __FUNCTION__);
	return;
}

