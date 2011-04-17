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

extern ProtocolManager *OstProtocolManager;

int myport;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Drone drone;
    OstProtocolManager = new ProtocolManager();

    app.setApplicationName(drone.objectName());

    if (argc > 1)
        myport = atoi(argv[1]);

    if (!drone.init())
        exit(-1);

    drone.setWindowFlags(drone.windowFlags()
        | Qt::WindowMaximizeButtonHint 
        | Qt::WindowMinimizeButtonHint);
    drone.showMinimized();
    app.exec();
    return 0;
} 

