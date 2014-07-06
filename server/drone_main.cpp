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

#include "../common/protocolmanager.h"

#include <google/protobuf/stubs/common.h>

#include <QCoreApplication>

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

extern ProtocolManager *OstProtocolManager;
extern char *version;
extern char *revision;

int myport;

void cleanup(int /*signum*/)
{
    QCoreApplication::instance()->exit(-1);
}

int main(int argc, char *argv[])
{
    int exitCode = 0;
    QCoreApplication app(argc, argv);
    Drone *drone = new Drone();
    OstProtocolManager = new ProtocolManager();

    app.setApplicationName(drone->objectName());

    // TODO: command line options
    // -v (--version)
    // -h (--help)
    // -p (--portnum)
    if (argc > 1)
        myport = atoi(argv[1]);

    if (!drone->init())
    {
        exitCode = -1;
        goto _exit;
    }

    qDebug("Version: %s", version);
    qDebug("Revision: %s", revision);

#ifdef Q_OS_UNIX
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup;
    if (sigaction(SIGTERM, &sa, NULL))
        qDebug("Failed to install SIGTERM handler. Cleanup may not happen!!!");
    if (sigaction(SIGINT, &sa, NULL))
        qDebug("Failed to install SIGINT handler. Cleanup may not happen!!!");
#endif

    exitCode = app.exec();

_exit:
    delete drone;
    delete OstProtocolManager;

    google::protobuf::ShutdownProtobufLibrary();

    return exitCode;
} 

