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
#include "params.h"
#include "rpcserver.h"
#include "settings.h"
#include "../common/updater.h"

#include <QMetaType>

extern Params appParams;
extern const char* version;
extern const char* revision;

Drone::Drone(QObject *parent)
     : QObject(parent)
{
    Updater *updater = new Updater();

#ifdef QT_DEBUG
    bool enableLogs = true;
#else
    bool enableLogs = !appParams.optLogsDisabled();
#endif

    rpcServer = new RpcServer(enableLogs);
    service = new MyService();

    connect(updater, SIGNAL(newVersionAvailable(QString)),
            this, SLOT(onNewVersion(QString)));
    updater->checkForNewVersion();

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

    if (!rpcServer->registerService(service, address, appParams.servicePortNumber()))
    {
        //qCritical(qPrintable(rpcServer->errorString()));
        return false;
    }

    connect(service, SIGNAL(notification(int, SharedProtobufMessage)), 
            rpcServer, SIGNAL(notifyClients(int, SharedProtobufMessage)));

    return true;
}

MyService* Drone::rpcService()
{
    return service;
}

void Drone::onNewVersion(QString newVersion)
{
    qWarning("%s", qPrintable(QString("New Ostinato version %1 available. "
                "Visit http://ostinato.org to download").arg(newVersion)));
}


