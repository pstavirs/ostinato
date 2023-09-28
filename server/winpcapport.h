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

#ifndef _SERVER_WIN_PCAP_PORT_H
#define _SERVER_WIN_PCAP_PORT_H

#include <QtGlobal>

#ifdef Q_OS_WIN32

#include "pcapport.h"

// BPF_MAJOR_VERSION definition is needed to prevent redefinition of
// bpf_program, bpf_insn etc. - this was not needed in WinPcap as it
// defines it, but npcap doesn't
#define BPF_MAJOR_VERSION 1
#include <packet32.h>

#include <ws2ipdef.h>
#include <iphlpapi.h>

class WinPcapPort : public PcapPort
{
public:
    WinPcapPort(int id, const char *device, const char *description);
    ~WinPcapPort();

    void init();
    virtual OstProto::LinkState linkState();
    virtual bool hasExclusiveControl();
    virtual bool setExclusiveControl(bool exclusive);

    static void fetchHostNetworkInfo();
    static void freeHostNetworkInfo();

protected:
    class PortMonitor: public PcapPort::PortMonitor 
    {
    public:
        PortMonitor(const char *device, Direction direction,
                AbstractPort::PortStats *stats);
        void run();
    };

    class StatsMonitor: public QThread
    {
        public:
            StatsMonitor();
            ~StatsMonitor();
            void run();
            void stop();
            bool waitForSetupFinished(int msecs = 10000);
        private:
            // TODO: int setPromisc(const char* portName);

            static const int kRefreshFreq_ = 1; // in seconds
            bool stop_;
            bool setupDone_;
    };

    bool isPromisc_{false};
    bool clearPromiscAtExit_{false};
    pcap_t *promiscHandle_{nullptr};

    static QList<WinPcapPort*> allPorts_;
    static StatsMonitor *monitor_; // rx/tx stats for ALL ports
    static bool internalPortStats_;

private:
    void populateInterfaceInfo();

    bool setPromisc();
    bool clearPromisc();

    LPADAPTER adapter_;
    NET_LUID luid_;
    PPACKET_OID_DATA linkStateOid_ ;

    static PIP_ADAPTER_ADDRESSES adapterList_;
};

#endif

#endif
