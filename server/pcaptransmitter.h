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

#include <QThread>
#include <pcap.h>

class PortTransmitter: public QThread
{
public:
    PortTransmitter(const char *device);
    ~PortTransmitter();

    bool setRateAccuracy(AbstractPort::Accuracy accuracy);

    void clearPacketList();
    void loopNextPacketSet(qint64 size, qint64 repeats,
        long repeatDelaySec, long repeatDelayNsec);
    bool appendToPacketList(long sec, long usec, const uchar *packet,
        int length);
    void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay) {
        returnToQIdx_ = loop ? 0 : -1;
        loopDelay_ = secDelay*long(1e6) + nsecDelay/1000;
    }
    void setHandle(pcap_t *handle);
    void useExternalStats(AbstractPort::PortStats *stats);
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

    class PacketSequence
    {
    public:
        PacketSequence() {
            sendQueue_ = pcap_sendqueue_alloc(1*1024*1024);
            lastPacket_ = NULL;
            packets_ = 0;
            bytes_ = 0;
            usecDuration_ = 0;
            repeatCount_ = 1;
            repeatSize_ = 1;
            usecDelay_ = 0;
        }
        ~PacketSequence() {
            pcap_sendqueue_destroy(sendQueue_);
        }
        bool hasFreeSpace(int size) {
            if ((sendQueue_->len + size) <= sendQueue_->maxlen)
                return true;
            else
                return false;
        }
        int appendPacket(const struct pcap_pkthdr *pktHeader,
                const uchar *pktData) {
            if (lastPacket_)
            {
                usecDuration_ += (pktHeader->ts.tv_sec
                                    - lastPacket_->ts.tv_sec) * long(1e6);
                usecDuration_ += (pktHeader->ts.tv_usec
                                    - lastPacket_->ts.tv_usec);
            }
            packets_++;
            bytes_ += pktHeader->caplen;
            lastPacket_ = (struct pcap_pkthdr *)
                                (sendQueue_->buffer + sendQueue_->len);
            return pcap_sendqueue_queue(sendQueue_, pktHeader, pktData);
        }
        pcap_send_queue *sendQueue_;
        struct pcap_pkthdr *lastPacket_;
        long packets_;
        long bytes_;
        ulong usecDuration_;
        int repeatCount_;
        int repeatSize_;
        long usecDelay_;
    };

    static void udelay(unsigned long usec);
    int sendQueueTransmit(pcap_t *p, pcap_send_queue *queue, long &overHead,
                int sync);

    QList<PacketSequence*> packetSequenceList_;
    PacketSequence *currentPacketSequence_;
    int repeatSequenceStart_;
    quint64 repeatSize_;
    quint64 packetCount_;

    int returnToQIdx_;
    quint64 loopDelay_;

    void (*udelayFn_)(unsigned long);

    bool usingInternalStats_;
    AbstractPort::PortStats *stats_;
    bool usingInternalHandle_;
    pcap_t *handle_;
    volatile bool stop_;
    volatile State state_;
};

#endif

