#include "portmanager.h"

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
      
#ifdef Q_OS_WIN32
        port = new WinPcapPort(i, device->name);
#else
        port = new PcapPort(i, device->name);
#endif

        port->init();
        portList_.append(port);

        qDebug("%d. %s", i, device->name);
        if (device->description)
            qDebug(" (%s)\n", device->description);
    }

    pcap_freealldevs(deviceList);
    
    return;
}

PortManager::~PortManager()
{
}

PortManager* PortManager::instance()
{
    if (!instance_)
        instance_ = new PortManager;

    return instance_;       
}
