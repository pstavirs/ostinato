/*
Copyright (C) 2010, 2014 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "rpcconn.h"

#include "pbqtio.h"
#include "pbrpccommon.h"
#include "pbrpccontroller.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <QHostAddress>
#include <QTcpSocket>
#include <qendian.h>

RpcConnection::RpcConnection(int socketDescriptor, 
                             ::google::protobuf::Service *service)
    : socketDescriptor(socketDescriptor),
      service(service)
{
    inStream = NULL;
    outStream = NULL;

    isPending = false;
    pendingMethodId = -1; // don't care as long as isPending is false

    parsing = false;
}

RpcConnection::~RpcConnection()
{ 
    qDebug("destroying connection to %s: %d", 
            clientSock->peerAddress().toString().toAscii().constData(),
            clientSock->peerPort());

    // If still connected, disconnect 
    if (clientSock->state() != QAbstractSocket::UnconnectedState) {
        clientSock->disconnectFromHost();
        clientSock->waitForDisconnected();
    }

    delete inStream;
    delete outStream;

    delete clientSock;
}

void RpcConnection::start()
{
    clientSock = new QTcpSocket;
    if (!clientSock->setSocketDescriptor(socketDescriptor)) {
        qWarning("Unable to initialize TCP socket for incoming connection");
        return;
    }
    qDebug("clientSock Thread = %p", clientSock->thread());

    qDebug("accepting new connection from %s: %d", 
            clientSock->peerAddress().toString().toAscii().constData(),
            clientSock->peerPort());
    inStream = new google::protobuf::io::CopyingInputStreamAdaptor(
                            new PbQtInputStream(clientSock));
    inStream->SetOwnsCopyingStream(true);
    outStream = new google::protobuf::io::CopyingOutputStreamAdaptor(
                            new PbQtOutputStream(clientSock));
    outStream->SetOwnsCopyingStream(true);

    connect(clientSock, SIGNAL(readyRead()), 
        this, SLOT(on_clientSock_dataAvail()));
    connect(clientSock, SIGNAL(disconnected()), 
        this, SLOT(on_clientSock_disconnected()));
    connect(clientSock, SIGNAL(error(QAbstractSocket::SocketError)), 
        this, SLOT(on_clientSock_error(QAbstractSocket::SocketError)));
}

void RpcConnection::sendRpcReply(PbRpcController *controller)
{
    google::protobuf::Message *response = controller->response();
    QIODevice *blob;
    char msgBuf[PB_HDR_SIZE];
    char* const msg = &msgBuf[0];
    int len;

    if (controller->Failed())
    {
        qDebug("rpc failed");
        goto _exit;
    }

    blob = controller->binaryBlob();
    if (blob)
    {
        len = blob->size();
        qDebug("is binary blob of len %d", len);

        *((quint16*)(msg+0)) = qToBigEndian(quint16(PB_MSG_TYPE_BINBLOB)); // type
        *((quint16*)(msg+2)) = qToBigEndian(quint16(pendingMethodId)); // method
        (*(quint32*)(msg+4)) = qToBigEndian(quint32(len)); // len

        clientSock->write(msg, PB_HDR_SIZE);

        blob->seek(0);
        while (!blob->atEnd())
        {    
            int l;

            len = blob->read(msg, sizeof(msgBuf));
            l = clientSock->write(msg, len);
            Q_ASSERT(l == len);
        }

        goto _exit;
    }

    if (!response->IsInitialized())
    {
        qWarning("response missing required fields!! <----");
        qDebug("response = \n%s"
               "missing = \n%s---->",
                response->DebugString().c_str(),
                response->InitializationErrorString().c_str());
        qFatal("exiting");
        goto _exit;
    }

    len = response->ByteSize();

    *((quint16*)(msg+0)) = qToBigEndian(quint16(PB_MSG_TYPE_RESPONSE)); // type
    *((quint16*)(msg+2)) = qToBigEndian(quint16(pendingMethodId)); // method
    *((quint32*)(msg+4)) = qToBigEndian(quint32(len)); // len

    // Avoid printing stats since it happens once every couple of seconds
    if (pendingMethodId != 13)
    {
        qDebug("Server(%s): sending %d bytes to client <----",
            __FUNCTION__, len + PB_HDR_SIZE);
        BUFDUMP(msg, len + 8);
        qDebug("method = %d\nreq = \n%s---->", 
            pendingMethodId, response->DebugString().c_str());
    }

    clientSock->write(msg, PB_HDR_SIZE);
    response->SerializeToZeroCopyStream(outStream);
    outStream->Flush();

_exit:
    delete controller;
    isPending = false;
}

void RpcConnection::on_clientSock_disconnected()
{
    qDebug("connection closed from %s: %d",
            clientSock->peerAddress().toString().toAscii().constData(),
            clientSock->peerPort());

    deleteLater();
    emit closed();
}

void RpcConnection::on_clientSock_error(QAbstractSocket::SocketError socketError)
{
    qDebug("%s (%d)", clientSock->errorString().toAscii().constData(), 
            socketError);
}

void RpcConnection::on_clientSock_dataAvail()
{
    uchar    msg[PB_HDR_SIZE];
    int        msgLen;
#if 0
    static bool parsing = false;
    static quint16    type, method;
    static quint32 len;
#endif
    const ::google::protobuf::MethodDescriptor    *methodDesc;
    ::google::protobuf::Message    *req, *resp;
    PbRpcController *controller;

    if (!parsing)
    {
        if (clientSock->bytesAvailable() < PB_HDR_SIZE)
            return;

        msgLen = clientSock->read((char*)msg, PB_HDR_SIZE);

        Q_ASSERT(msgLen == PB_HDR_SIZE);

        type = qFromBigEndian<quint16>(&msg[0]);
        method = qFromBigEndian<quint16>(&msg[2]);
        len = qFromBigEndian<quint32>(&msg[4]);
        //qDebug("type = %d, method = %d, len = %d", type, method, len);

        parsing = true;
    }

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

    if (len) {
        bool ok = req->ParseFromBoundedZeroCopyStream(inStream, len);
        if (!ok)
            qWarning("ParseFromBoundedZeroCopyStream fail "
                     "for method %d and len %d", method, len);
    }

    if (!req->IsInitialized())
    {
        qWarning("Missing required fields in request <----");
        qDebug("method = %d\n"
               "req = \n%s"
               "missing = \n%s----->",
                method, req->DebugString().c_str(),
                req->InitializationErrorString().c_str());
        qFatal("exiting");
        delete req;
        delete resp;

        goto _error_exit2;
    }
    
    if (method != 13) {
        qDebug("Server(%s): successfully received/parsed msg <----", __FUNCTION__);
        qDebug("method = %d\n"
               "req = \n%s---->",
                method,
                req->DebugString().c_str());
    }

    controller = new PbRpcController(req, resp);

    //qDebug("before service->callmethod()");

    service->CallMethod(methodDesc, controller, req, resp,
        google::protobuf::NewCallback(this, &RpcConnection::sendRpcReply, 
                                      controller));

    parsing = false;

    return;

_error_exit:
    inStream->Skip(len);
_error_exit2:
    parsing = false;
    qDebug("server(%s): discarding msg from client", __FUNCTION__);
    return;
}

