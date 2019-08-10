/*
Copyright (C) 2016 Srivats P.

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

#ifndef _PCAP_RX_STATS_H
#define _PCAP_RX_STATS_H

#include "streamstats.h"

#include "pcapsession.h"

class PcapRxStats: public PcapSession
{
public:
    PcapRxStats(const char *device, StreamStats &portStreamStats);
    pcap_t* handle();
    void run();
    bool start();
    bool stop();
    bool isRunning();
    bool isDirectional();

private:
    enum State {
        kNotStarted,
        kRunning,
        kFinished
    };

    QString device_;
    StreamStats &streamStats_;
    volatile bool stop_;
    pcap_t *handle_;
    volatile State state_;
    bool isDirectional_;
};

#endif
