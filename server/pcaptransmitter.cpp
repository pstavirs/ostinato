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
        const char *device)
    : txThread_(device)
{
    adjustRxStreamStats_ = false;
    txStats_.setObjectName(QString("TxStats:%1").arg(device));
    memset(&stats_, 0, sizeof(stats_));
    txStats_.setTxThreadStats(&stats_);

    txThread_.setStats(&stats_);
    connect(&txThread_, SIGNAL(finished()), SLOT(updateTxThreadStreamStats()));
}

PcapTransmitter::~PcapTransmitter()
{
    if (txThread_.isRunning())
        txThread_.stop();
    if (txStats_.isRunning())
        txStats_.stop();
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

// XXX: Stats are reset on read
void PcapTransmitter::updateTxRxStreamStats(StreamStats &streamStats)
{
    QMutexLocker lock(&streamStatsLock_);
    StreamStatsIterator i(streamStats_);

    while (i.hasNext())
    {
        i.next();
        uint guid = i.key();
        StreamStatsTuple sst = i.value();

        streamStats[guid].tx_pkts += sst.tx_pkts;
        streamStats[guid].tx_bytes += sst.tx_bytes;
        if (adjustRxStreamStats_) {
            // XXX: rx_pkts counting may lag behind tx_pkts, so stream stats
            // may become negative after adjustment transiently. But this
            // should fix itself once all the rx pkts come in
            streamStats[guid].rx_pkts -= sst.tx_pkts;
            streamStats[guid].rx_bytes -= sst.tx_bytes;
        }
    }
    streamStats_.clear();
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

bool PcapTransmitter::setPacketListTtagMarkers(
        QList<uint> markers,
        uint repeatInterval)
{
    return txThread_.setPacketListTtagMarkers(markers, repeatInterval);
}

void PcapTransmitter::useExternalStats(AbstractPort::PortStats *stats)
{
    txStats_.useExternalStats(stats);
}

void PcapTransmitter::start()
{
    // XXX: Start the stats thread before the tx thread, so no tx stats
    // is missed
    txStats_.start();
    Q_ASSERT(txStats_.isRunning());
    txThread_.start();
}

void PcapTransmitter::stop()
{
    // XXX: Stop the tx thread before the stats thread, so no tx stats
    // is missed
    txThread_.stop();
    Q_ASSERT(!txThread_.isRunning());
    txStats_.stop();
}

bool PcapTransmitter::isRunning()
{
    return txThread_.isRunning();
}

double PcapTransmitter::lastTxDuration()
{
    return txThread_.lastTxDuration();
}

void PcapTransmitter::updateTxThreadStreamStats()
{
    QMutexLocker lock(&streamStatsLock_);
    PcapTxThread *txThread = dynamic_cast<PcapTxThread*>(sender());
    StreamStats threadStreamStats = txThread->streamStats();
    StreamStatsIterator i(threadStreamStats);

    while (i.hasNext())
    {
        i.next();
        uint guid = i.key();
        StreamStatsTuple sst = i.value();

        streamStats_[guid].tx_pkts += sst.tx_pkts;
        streamStats_[guid].tx_bytes += sst.tx_bytes;
    }
}
