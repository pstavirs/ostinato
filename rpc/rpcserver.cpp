/*
Copyright (C) 2010 Srivats P.

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

#include "rpcthread.h"

RpcServer::RpcServer()
{
    service = NULL; 
}

RpcServer::~RpcServer()
{ 
}

bool RpcServer::registerService(::google::protobuf::Service *service, 
    quint16 tcpPortNum)
{
    this->service = service;

    if (!listen(QHostAddress::Any, tcpPortNum)) 
    {
        qDebug("Unable to start the server: %s", 
                errorString().toAscii().constData());
        return false;
    }

    qDebug("The server is running on %s: %d", 
            serverAddress().toString().toAscii().constData(),
            serverPort());
    return true;
}

void RpcServer::incomingConnection(int socketDescriptor)
{
    RpcThread *thread = new RpcThread(socketDescriptor, service, this);

    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
