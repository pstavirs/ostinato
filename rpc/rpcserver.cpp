//#include "pbhelper.h"
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
        qDebug("Unable to start the server: %s", 
                server->errorString().toAscii().constData());
        errorString_ = QString("Error starting Ostinato server: %1").arg(
                server->errorString());
        return false;
    }

    qDebug("The server is running on %s: %d", 
            server->serverAddress().toString().toAscii().constData(),
            server->serverPort());
    errorString_ = QString();
    return true;
}

QString RpcServer::errorString()
{
    return errorString_;
}

void RpcServer::done(::google::protobuf::Message *resp, PbRpcController *PbRpcController)
{
    QIODevice    *blob;
    char    msg[MSGBUF_SIZE];
    int        len;

    //qDebug("In RpcServer::done");

    if (PbRpcController->Failed())
    {
        qDebug("rpc failed");
        goto _exit;
    }

    blob = PbRpcController->binaryBlob();
    if (blob)
    {
        len = blob->size();
        qDebug("is binary blob of len %d", len);

        *((quint16*)(&msg[0])) = HTONS(PB_MSG_TYPE_BINBLOB); // type
        *((quint16*)(&msg[2])) = HTONS(pendingMethodId); // method
        (*(quint32*)(&msg[4])) = HTONL(len); // len

        clientSock->write(msg, PB_HDR_SIZE);

        blob->seek(0);
        while (!blob->atEnd())
        {    
            int l;

            len = blob->read(msg, sizeof(msg));
            l = clientSock->write(msg, len);
            Q_ASSERT(l == len);
        }

        goto _exit;
    }

    if (!resp->IsInitialized())
    {
        qWarning("response missing required fields!!");
        qDebug(resp->InitializationErrorString().c_str());
        qFatal("exiting");
        goto _exit;
    }

    resp->SerializeToArray((void*) &msg[PB_HDR_SIZE], sizeof(msg));

    len = resp->ByteSize();

    *((quint16*)(&msg[0])) = HTONS(PB_MSG_TYPE_RESPONSE); // type
    *((quint16*)(&msg[2])) = HTONS(pendingMethodId); // method
    *((quint32*)(&msg[4])) = HTONL(len); // len

    // Avoid printing stats since it happens once every couple of seconds
    if (pendingMethodId != 12)
    {
        qDebug("Server(%s): sending %d bytes to client encoding <%s>", 
            __FUNCTION__, len + PB_HDR_SIZE, resp->DebugString().c_str());
        //BUFDUMP(msg, len + 8);
    }

    clientSock->write(msg, PB_HDR_SIZE + len);

_exit:
    delete PbRpcController;
    isPending = false;
}

void RpcServer::when_newConnection()
{
    if (clientSock)
    {
        QTcpSocket    *sock;

        qDebug("already connected, no new connections will be accepted");

        // Accept and close connection
        //! \todo (MED) Send reason msg to client
        sock = server->nextPendingConnection();
        sock->disconnectFromHost();
        sock->deleteLater();
        goto _exit;
    }

    clientSock = server->nextPendingConnection();
    qDebug("accepting new connection from %s: %d", 
            clientSock->peerAddress().toString().toAscii().constData(),
            clientSock->peerPort());

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
    qDebug("connection closed from %s: %d",
            clientSock->peerAddress().toString().toAscii().constData(),
            clientSock->peerPort());

    clientSock->deleteLater();
    clientSock = NULL;
}

void RpcServer::when_error(QAbstractSocket::SocketError socketError)
{
    qDebug("%s", clientSock->errorString().toAscii().constData());
}

void RpcServer::when_dataAvail()
{
    char    msg[MSGBUF_SIZE];
    int        msgLen;
    static bool parsing = false;
    static quint16    type, method;
    static quint32 len;
    const ::google::protobuf::MethodDescriptor    *methodDesc;
    ::google::protobuf::Message    *req, *resp;
    PbRpcController    *controller;

    if (!parsing)
    {
        if (clientSock->bytesAvailable() < PB_HDR_SIZE)
            return;

        msgLen = clientSock->read(msg, PB_HDR_SIZE);

        Q_ASSERT(msgLen == PB_HDR_SIZE);

        type = NTOHS(GET16(&msg[0]));
        method = NTOHS(GET16(&msg[2]));
        len = NTOHL(GET32(&msg[4]));
        //qDebug("type = %d, method = %d, len = %d", type, method, len);

        parsing = true;
    }

    if (clientSock->bytesAvailable() < len)
        return;

    msgLen = clientSock->read(msg, sizeof(msg));
    Q_ASSERT((unsigned) msgLen == len);

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
        goto _error_exit; //! \todo Return Error to client
    }

    if (isPending)
    {
        qDebug("server(%s): rpc pending, try again", __FUNCTION__);
        goto _error_exit; //! \todo Return Error to client
    }

    pendingMethodId = method;
    isPending = true;

    req = service->GetRequestPrototype(methodDesc).New();
    resp = service->GetResponsePrototype(methodDesc).New();

    req->ParseFromArray((void*)msg, len);
    if (!req->IsInitialized())
    {
        qWarning("Missing required fields in request");
        qDebug(req->InitializationErrorString().c_str());
        qFatal("exiting");
        delete req;
        delete resp;

        goto _error_exit;
    }
    //qDebug("Server(%s): successfully parsed as <%s>", __FUNCTION__, 
        //resp->DebugString().c_str());

    controller = new PbRpcController;

    //qDebug("before service->callmethod()");

    service->CallMethod(methodDesc, controller, req, resp,
        NewCallback(this, &RpcServer::done, resp, controller));

    parsing = false;

    return;

_error_exit:
    parsing = false;
    qDebug("server(%s): discarding msg from client", __FUNCTION__);
    return;
}

