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

#ifndef _RPC_THREAD_H
#define _RPC_THREAD_H

#include <QThread>
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

class RpcThread : public QThread
{
    Q_OBJECT

public:
    RpcThread(int socketDescriptor, 
              ::google::protobuf::Service *service,
              QObject *parent);
    virtual ~RpcThread();
    void run();

private:
    QString errorString(); // FIXME: needed? why?
    void done(PbRpcController *controller);

private slots:
    void when_disconnected();
    void when_dataAvail();
    void when_error(QAbstractSocket::SocketError socketError);

private:
    int socketDescriptor;
    QTcpSocket *clientSock;

    ::google::protobuf::Service *service;
    ::google::protobuf::io::CopyingInputStreamAdaptor  *inStream;
    ::google::protobuf::io::CopyingOutputStreamAdaptor *outStream;

    bool isPending;
    int pendingMethodId;
    QString errorString_;
};

#endif
