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

#ifndef _PACKET_SEQUENCE_H
#define _PACKET_SEQUENCE_H

#include "pcapextra.h"

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

#endif
