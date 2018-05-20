/*
Copyright (C) 2010-2016 Srivats P.

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

#ifndef _PCAP_TRANSMITTER_H
#define _PCAP_TRANSMITTER_H

#include "abstractport.h"
#include "pcaptxstats.h"
#include "pcaptxthread.h"
#include "statstuple.h"

class PcapTransmitter : QObject
{
    Q_OBJECT
public:
    PcapTransmitter(const char *device, StreamStats &portStreamStats);
    ~PcapTransmitter();

    bool setRateAccuracy(AbstractPort::Accuracy accuracy);
    bool setStreamStatsTracking(bool enable);
    void adjustRxStreamStats(bool enable);

    void clearPacketList();
    void loopNextPacketSet(qint64 size, qint64 repeats,
                           long repeatDelaySec, long repeatDelayNsec);
    bool appendToPacketList(long sec, long usec, const uchar *packet,
                            int length);
    void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay);

    void setHandle(pcap_t *handle);
    void useExternalStats(AbstractPort::PortStats *stats);

    void start();
    void stop();
    bool isRunning();
private slots:
    void updateTxThreadStreamStats();
private:
    StreamStats &streamStats_;
    PcapTxThread txThread_;
    PcapTxStats txStats_;
    StatsTuple stats_;
    bool adjustRxStreamStats_;
};

#endif

