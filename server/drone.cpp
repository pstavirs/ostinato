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

#include "rpcserver.h"
#include "myservice.h"

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
    Q_ASSERT(rpcServer);

    if (!rpcServer->registerService(service, myport ? myport : 7878))
    {
        //qCritical(qPrintable(rpcServer->errorString()));
        return false;
    }

    return true;
}
