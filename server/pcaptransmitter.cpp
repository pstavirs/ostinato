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

#include "pcaptransmitter.h"

PcapTransmitter::PcapTransmitter(
        const char *device,
        StreamStats &portStreamStats)
    : streamStats_(portStreamStats), txThread_(device)
{
    adjustRxStreamStats_ = false;
    memset(&stats_, 0, sizeof(stats_));
    txStats_.setTxThreadStats(&stats_);
    txStats_.start(); // TODO: alongwith user transmit start

    txThread_.setStats(&stats_);
    connect(&txThread_, SIGNAL(finished()), SLOT(updateTxThreadStreamStats()));
}

PcapTransmitter::~PcapTransmitter()
{
    txStats_.stop(); // TODO: alongwith user transmit stop
}

bool PcapTransmitter::setRateAccuracy(
        AbstractPort::Accuracy accuracy)
{
    return txThread_.setRateAccuracy(accuracy);
}

void PcapTransmitter::adjustRxStreamStats(bool enable)
{
    adjustRxStreamStats_ = enable;
}

bool PcapTransmitter::setStreamStatsTracking(bool enable)
{
    return txThread_.setStreamStatsTracking(enable);
}

void PcapTransmitter::clearPacketList()
{
    txThread_.clearPacketList();
}

void PcapTransmitter::loopNextPacketSet(
        qint64 size,
        qint64 repeats,
        long repeatDelaySec,
        long repeatDelayNsec)
{
    txThread_.loopNextPacketSet(size, repeats, repeatDelaySec, repeatDelayNsec);
}

bool PcapTransmitter::appendToPacketList(long sec, long nsec,
        const uchar *packet, int length)
{
    return txThread_.appendToPacketList(sec, nsec, packet, length);
}

void PcapTransmitter::setHandle(pcap_t *handle)
{
    txThread_.setHandle(handle);
}

void PcapTransmitter::setPacketListLoopMode(
        bool loop,
        quint64 secDelay,
        quint64 nsecDelay)
{
    txThread_.setPacketListLoopMode(loop, secDelay, nsecDelay);
}

void PcapTransmitter::useExternalStats(AbstractPort::PortStats *stats)
{
    txStats_.useExternalStats(stats);
}

void PcapTransmitter::start()
{
    txThread_.start();
}

void PcapTransmitter::stop()
{
    txThread_.stop();
}

bool PcapTransmitter::isRunning()
{
    return txThread_.isRunning();
}

void PcapTransmitter::updateTxThreadStreamStats()
{
    PcapTxThread *txThread = dynamic_cast<PcapTxThread*>(sender());
    const StreamStats& threadStreamStats = txThread->streamStats();
    StreamStatsIterator i(threadStreamStats);

    while (i.hasNext())
    {
        i.next();
        uint guid = i.key();
        StreamStatsTuple sst = i.value();

        streamStats_[guid].tx_pkts += sst.tx_pkts;
        streamStats_[guid].tx_bytes += sst.tx_bytes;
        if (adjustRxStreamStats_) {
            streamStats_[guid].rx_pkts -= sst.tx_pkts;
            streamStats_[guid].rx_bytes -= sst.tx_bytes;
        }
    }
    txThread->clearStreamStats();
}
