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

#include "drone.h"

#include "myservice.h"
#include "rpcserver.h"
#include "settings.h"

#include <QMetaType>

extern int myport;
extern const char* version;
extern const char* revision;

Drone::Drone(QObject *parent)
     : QObject(parent)
{
    rpcServer = new RpcServer();
    service = new MyService();
}

Drone::~Drone()
{
    delete rpcServer;
    delete service;
}

bool Drone::init()
{
    QString addr = appSettings->value(kRpcServerAddress).toString();
    QHostAddress address = addr.isEmpty() ?
        QHostAddress::Any : QHostAddress(addr);

    Q_ASSERT(rpcServer);

    qRegisterMetaType<SharedProtobufMessage>("SharedProtobufMessage");

    if (address.isNull()) {
        qWarning("Invalid RpcServer Address <%s> specified. Using 'Any'",
                qPrintable(addr));
        address = QHostAddress::Any;
    }

    if (!rpcServer->registerService(service, address, myport ? myport : 7878))
    {
        //qCritical(qPrintable(rpcServer->errorString()));
        return false;
    }

    connect(service, SIGNAL(notification(int, SharedProtobufMessage)), 
            rpcServer, SIGNAL(notifyClients(int, SharedProtobufMessage)));

    return true;
}
