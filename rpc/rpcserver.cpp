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

#include "rpcserver.h"

#include "rpcconn.h"

#include <QThread>

RpcServer::RpcServer(bool perConnLogs)
{
    service = NULL; 

    if (perConnLogs)
        qInstallMessageHandler(RpcConnection::connIdMsgHandler);
}

RpcServer::~RpcServer()
{ 
}

bool RpcServer::registerService(::google::protobuf::Service *service,
    QHostAddress address, quint16 tcpPortNum)
{
    this->service = service;

    if (!listen(address, tcpPortNum))
    {
        qDebug("Unable to start the server on <%s>: %s",
                qPrintable(address.toString()),
                qPrintable(errorString()));
        return false;
    }

    qDebug("The server is running on %s: %d", 
            qPrintable(serverAddress().toString()),
            serverPort());
    return true;
}

void RpcServer::incomingConnection(qintptr socketDescriptor)
{
    QThread *thread = new QThread;
    RpcConnection *conn = new RpcConnection(socketDescriptor, service);

    conn->moveToThread(thread);

    connect(thread, SIGNAL(started()), conn, SLOT(start()));

    // NOTE: conn "self-destructs" after emitting closed
    // use 'closed' to stop execution of the thread
    connect(conn, SIGNAL(closed()), thread, SLOT(quit()));

    // setup thread to "self-destruct" when it is done
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    connect(this, SIGNAL(notifyClients(int, SharedProtobufMessage)),
            conn, SLOT(sendNotification(int, SharedProtobufMessage)));

    thread->start();
}
