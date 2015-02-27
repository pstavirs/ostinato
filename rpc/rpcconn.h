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

#ifndef _RPC_CONNECTION_H
#define _RPC_CONNECTION_H

#include <QAbstractSocket>

// forward declarations
class PbRpcController;
class QTcpSocket;
namespace google {
    namespace protobuf {
        class Service;
        namespace io {
            class CopyingInputStreamAdaptor;
            class CopyingOutputStreamAdaptor;
        }
    }
}

class RpcConnection : public QObject
{
    Q_OBJECT

public:
    RpcConnection(int socketDescriptor, ::google::protobuf::Service *service);
    virtual ~RpcConnection();
    static void connIdMsgHandler(QtMsgType type, const char* msg);

private:
    void writeHeader(char* header, quint16 type, quint16 method, 
                     quint32 length);
    void sendRpcReply(PbRpcController *controller);

signals:
    void closed();

private slots:
    void start();
    void on_clientSock_dataAvail();
    void on_clientSock_error(QAbstractSocket::SocketError socketError);
    void on_clientSock_disconnected();

private:
    int socketDescriptor;
    QTcpSocket *clientSock;

    ::google::protobuf::Service *service;
    ::google::protobuf::io::CopyingInputStreamAdaptor  *inStream;
    ::google::protobuf::io::CopyingOutputStreamAdaptor *outStream;

    bool isPending;
    int pendingMethodId;

    bool isCompatCheckDone;
};

#endif
