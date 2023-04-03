/*
Copyright (C) 2023 Srivats P.

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

#ifndef _PCAP_TX_TTAG_H
#define _PCAP_TX_TTAG_H

#include "pcapsession.h"

class StreamTiming;

class PcapTxTtagStats: public PcapSession
{
public:
    PcapTxTtagStats(const char *device, int id);

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
    bool isDirectional_{true};
    pcap_t *handle_{nullptr};
    volatile State state_{kNotStarted};
    volatile bool stop_{false};

    int portId_;

    StreamTiming *timing_{nullptr};
};

#endif
