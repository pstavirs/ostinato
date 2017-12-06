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

#ifndef _PCAP_TX_THREAD_H
#define _PCAP_TX_THREAD_H

#include "abstractport.h"
#include "packetsequence.h"
#include "statstuple.h"

#include <QThread>
#include <pcap.h>

class PcapTxThread: public QThread
{
public:
    PcapTxThread(const char *device);
    ~PcapTxThread();

    bool setRateAccuracy(AbstractPort::Accuracy accuracy);
    bool setStreamStatsTracking(bool enable);

    void clearPacketList();
    void loopNextPacketSet(qint64 size, qint64 repeats,
                           long repeatDelaySec, long repeatDelayNsec);
    bool appendToPacketList(long sec, long usec, const uchar *packet,
                            int length);
    void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay);

    void setHandle(pcap_t *handle);

    void setStats(StatsTuple *stats);

    const StreamStats& streamStats();
    void clearStreamStats();

    void run();

    void start();
    void stop();
    bool isRunning();

private:
    enum State
    {
        kNotStarted,
        kRunning,
        kFinished
    };

    static void udelay(unsigned long usec);
    int sendQueueTransmit(pcap_t *p, pcap_send_queue *queue, long &overHead,
                int sync);
    void updateStreamStats();

    // Intermediate state variables used while building the packet list
    PacketSequence *currentPacketSequence_;
    int repeatSequenceStart_;
    quint64 repeatSize_;
    quint64 packetCount_;

    QList<PacketSequence*> packetSequenceList_;
    quint64 packetListSize_; // count of pkts in packet List including repeats

    int returnToQIdx_;
    quint64 loopDelay_;

    void (*udelayFn_)(unsigned long);

    bool usingInternalHandle_;
    pcap_t *handle_;
    volatile bool stop_;
    volatile State state_;

    bool trackStreamStats_;
    StatsTuple *stats_;
    StatsTuple lastStats_;
    StreamStats streamStats_;
};

#endif

