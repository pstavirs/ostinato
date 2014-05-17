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

#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#if 0
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <QTcpSocket>

#include "pbrpccommon.h"
#include "pbrpccontroller.h"
#endif

#include <QTcpServer>

// forward declaration
namespace google {
    namespace protobuf {
        class Service;
    }
}

class RpcServer : public QTcpServer
{
    Q_OBJECT

#if 0
    QTcpServer *server;
    QTcpSocket *clientSock;

    ::google::protobuf::io::CopyingInputStreamAdaptor  *inStream;
    ::google::protobuf::io::CopyingOutputStreamAdaptor *outStream;

    bool isPending;
    int pendingMethodId;
    QString errorString_;
#endif

public:
    RpcServer();    //! \todo (LOW) use 'parent' param
    virtual ~RpcServer();

    bool registerService(::google::protobuf::Service *service,
        quint16 tcpPortNum);

protected:
    void incomingConnection(int socketDescriptor);

#if 0
    QString errorString();
    void done(PbRpcController *controller);

private slots:
     void when_newConnection();
     void when_disconnected();
     void when_dataAvail();
    void when_error(QAbstractSocket::SocketError socketError);
#endif
private:
    ::google::protobuf::Service *service;
};

#endif
