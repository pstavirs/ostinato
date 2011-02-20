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

#include "portmanager.h"

#include <QtGlobal>
#include <pcap.h>

#include "pcapport.h"
#include "winpcapport.h"

PortManager *PortManager::instance_ = NULL;

PortManager::PortManager()
{
    int i;
    pcap_if_t *deviceList;
    pcap_if_t *device;
    char errbuf[PCAP_ERRBUF_SIZE];

    qDebug("Retrieving the device list from the local machine\n"); 

    if (pcap_findalldevs(&deviceList, errbuf) == -1)
        qDebug("Error in pcap_findalldevs_ex: %s\n", errbuf);

    for(device = deviceList, i = 0; device != NULL; device = device->next, i++)
    {
        AbstractPort *port;
      
        qDebug("%d. %s", i, device->name);
        if (device->description)
            qDebug(" (%s)\n", device->description);

#ifdef Q_OS_WIN32
        port = new WinPcapPort(i, device->name);
#else
        port = new PcapPort(i, device->name);
#endif

        if (!port->isUsable())
        {
            qDebug("%s: unable to open %s. Skipping!", __FUNCTION__,
                    device->name);
            delete port;
            i--;
            continue;
        }

        port->init();
        portList_.append(port);
    }

    pcap_freealldevs(deviceList);
    
    return;
}

PortManager::~PortManager()
{
    while (!portList_.isEmpty())
        delete portList_.takeFirst();
}

PortManager* PortManager::instance()
{
    if (!instance_)
        instance_ = new PortManager;

    return instance_;       
}
