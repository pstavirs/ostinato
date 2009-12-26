#ifndef _SERVER_WIN_PCAP_PORT_H
#define _SERVER_WIN_PCAP_PORT_H

#include "pcapport.h"

#include <packet32.h>

class WinPcapPort : public PcapPort
{
public:
    WinPcapPort(int id, const char *device);
    ~WinPcapPort();

    virtual OstProto::LinkState linkState();

protected:
    class PortMonitor: public PcapPort::PortMonitor 
    {
    public:
        PortMonitor(const char *device, Direction direction,
                AbstractPort::PortStats *stats);
        void run();
    };
private:
    LPADAPTER adapter_;
    PPACKET_OID_DATA linkStateOid_ ;
};

#endif

/* vim: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab: */
