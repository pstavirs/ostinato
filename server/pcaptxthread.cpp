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

#include "statstuple.h"
#include "timestamp.h"

PcapTxThread::PcapTxThread(const char *device)
{
    char errbuf[PCAP_ERRBUF_SIZE] = "";

#ifdef Q_OS_WIN32
    LARGE_INTEGER   freq;
    if (QueryPerformanceFrequency(&freq))
        gTicksFreq = freq.QuadPart;
    else
        Q_ASSERT_X(false, "PcapTxThread::PcapTxThread",
                "This Win32 platform does not support performance counter");
#endif
    state_ = kNotStarted;
    stop_ = false;
    trackStreamStats_ = false;
    clearPacketList();
    handle_ = pcap_open_live(device, 64 /* FIXME */, 0, 1000 /* ms */, errbuf);

    if (handle_ == NULL)
        goto _open_error;

    usingInternalHandle_ = true;

    stats_ = NULL;

    return;

_open_error:
    qDebug("%s: Error opening port %s: %s\n", __FUNCTION__, device, errbuf);
    usingInternalHandle_ = false;
}

PcapTxThread::~PcapTxThread()
{
    if (usingInternalHandle_)
        pcap_close(handle_);
}

bool PcapTxThread::setRateAccuracy(
        AbstractPort::Accuracy accuracy)
{
    switch (accuracy) {
    case AbstractPort::kHighAccuracy:
        udelayFn_ = udelay;
        qWarning("%s: rate accuracy set to High - busy wait", __FUNCTION__);
        break;
    case AbstractPort::kLowAccuracy:
        udelayFn_ = QThread::usleep;
        qWarning("%s: rate accuracy set to Low - usleep", __FUNCTION__);
        break;
    default:
        qWarning("%s: unsupported rate accuracy value %d", __FUNCTION__,
                accuracy);
        return false;
    }
    return true;
}

bool PcapTxThread::setStreamStatsTracking(bool enable)
{
    trackStreamStats_ = enable;
    return true;
}

void PcapTxThread::clearPacketList()
{
    Q_ASSERT(!isRunning());
    // \todo lock for packetSequenceList
    while(packetSequenceList_.size())
        delete packetSequenceList_.takeFirst();

    currentPacketSequence_ = NULL;
    repeatSequenceStart_ = -1;
    repeatSize_ = 0;
    packetCount_ = 0;

    packetListSize_ = 0;
    returnToQIdx_ = -1;

    setPacketListLoopMode(false, 0, 0);
}

void PcapTxThread::loopNextPacketSet(qint64 size, qint64 repeats,
        long repeatDelaySec, long repeatDelayNsec)
{
    currentPacketSequence_ = new PacketSequence(trackStreamStats_);
    currentPacketSequence_->repeatCount_ = repeats;
    currentPacketSequence_->usecDelay_ = repeatDelaySec * long(1e6)
                                            + repeatDelayNsec/1000;

    repeatSequenceStart_ = packetSequenceList_.size();
    repeatSize_ = size;
    packetCount_ = 0;

    packetSequenceList_.append(currentPacketSequence_);
}

bool PcapTxThread::appendToPacketList(long sec, long nsec,
        const uchar *packet, int length)
{
    bool op = true;
    pcap_pkthdr pktHdr;

    pktHdr.caplen = pktHdr.len = length;
    pktHdr.ts.tv_sec = sec;
    pktHdr.ts.tv_usec = nsec/1000;

    if (currentPacketSequence_ == NULL ||
            !currentPacketSequence_->hasFreeSpace(2*sizeof(pcap_pkthdr)+length))
    {
        if (currentPacketSequence_ != NULL)
        {
            long usecs;

            usecs = (pktHdr.ts.tv_sec
                        - currentPacketSequence_->lastPacket_->ts.tv_sec)
                            * long(1e6);
            usecs += (pktHdr.ts.tv_usec
                        - currentPacketSequence_->lastPacket_->ts.tv_usec);
            currentPacketSequence_->usecDelay_ = usecs;
        }

        //! \todo (LOW): calculate sendqueue size
        currentPacketSequence_ = new PacketSequence(trackStreamStats_);

        packetSequenceList_.append(currentPacketSequence_);

        // Validate that the pkt will fit inside the new currentSendQueue_
        Q_ASSERT(currentPacketSequence_->hasFreeSpace(
                    sizeof(pcap_pkthdr) + length));
    }

    if (currentPacketSequence_->appendPacket(&pktHdr, (u_char*) packet) < 0)
    {
        op = false;
    }

    packetCount_++;
    packetListSize_ += repeatSize_ ?
                                currentPacketSequence_->repeatCount_ : 1;
    if (repeatSize_ > 0 && packetCount_ == repeatSize_)
    {
        qDebug("repeatSequenceStart_=%d, repeatSize_ = %llu",
                repeatSequenceStart_, repeatSize_);

        // Set the packetSequence repeatSize
        Q_ASSERT(repeatSequenceStart_ >= 0);
        Q_ASSERT(repeatSequenceStart_ < packetSequenceList_.size());

        if (currentPacketSequence_ != packetSequenceList_[repeatSequenceStart_])
        {
            PacketSequence *start = packetSequenceList_[repeatSequenceStart_];

            currentPacketSequence_->usecDelay_ = start->usecDelay_;
            start->usecDelay_ = 0;
            start->repeatSize_ =
                    packetSequenceList_.size() - repeatSequenceStart_;
        }

        repeatSize_ = 0;

        // End current pktSeq and trigger a new pktSeq allocation for next pkt
        currentPacketSequence_ = NULL;
    }

    return op;
}

void PcapTxThread::setPacketListLoopMode(
        bool loop,
        quint64 secDelay,
        quint64 nsecDelay)
{
    returnToQIdx_ = loop ? 0 : -1;
    loopDelay_ = secDelay*long(1e6) + nsecDelay/1000;
}

void PcapTxThread::setHandle(pcap_t *handle)
{
    if (usingInternalHandle_)
        pcap_close(handle_);
    handle_ = handle;
    usingInternalHandle_ = false;
}

void PcapTxThread::setStats(StatsTuple *stats)
{
    stats_ = stats;
}

const StreamStats& PcapTxThread::streamStats()
{
    return streamStats_;
}

void PcapTxThread::clearStreamStats()
{
    streamStats_.clear();
}

void PcapTxThread::run()
{
    //! \todo (MED) Stream Mode - continuous: define before implement

    // NOTE1: We can't use pcap_sendqueue_transmit() directly even on Win32
    // 'coz of 2 reasons - there's no way of stopping it before all packets
    // in the sendQueue are sent out and secondly, stats are available only
    // when all packets have been sent - no periodic updates
    //
    // NOTE2: Transmit on the Rx Handle so that we can receive it back
    // on the Tx Handle to do stats
    //
    // NOTE3: Update pcapExtra counters - port TxStats will be updated in the
    // 'stats callback' function so that both Rx and Tx stats are updated
    // together

    const int kSyncTransmit = 1;
    int i;
    long overHead = 0; // overHead should be negative or zero

    qDebug("packetSequenceList_.size = %d", packetSequenceList_.size());
    if (packetSequenceList_.size() <= 0)
        goto _exit;

    for(i = 0; i < packetSequenceList_.size(); i++) {
        qDebug("sendQ[%d]: rptCnt = %d, rptSz = %d, usecDelay = %ld", i,
                packetSequenceList_.at(i)->repeatCount_,
                packetSequenceList_.at(i)->repeatSize_,
                packetSequenceList_.at(i)->usecDelay_);
        qDebug("sendQ[%d]: pkts = %ld, usecDuration = %ld", i,
                packetSequenceList_.at(i)->packets_,
                packetSequenceList_.at(i)->usecDuration_);
    }

    lastStats_ = *stats_; // used for stream stats

    state_ = kRunning;
    i = 0;
    while (i < packetSequenceList_.size())
    {

_restart:
        int rptSz  = packetSequenceList_.at(i)->repeatSize_;
        int rptCnt = packetSequenceList_.at(i)->repeatCount_;

        for (int j = 0; j < rptCnt; j++)
        {
            for (int k = 0; k < rptSz; k++)
            {
                int ret;
                PacketSequence *seq = packetSequenceList_.at(i+k);
#ifdef Q_OS_WIN32
                TimeStamp ovrStart, ovrEnd;

                if (seq->usecDuration_ <= long(1e6)) // 1s
                {
                    getTimeStamp(&ovrStart);
                    ret = pcap_sendqueue_transmit(handle_,
                            seq->sendQueue_, kSyncTransmit);
                    if (ret >= 0)
                    {
                        stats_->pkts += seq->packets_;
                        stats_->bytes += seq->bytes_;

                        getTimeStamp(&ovrEnd);
                        overHead += seq->usecDuration_
                            - udiffTimeStamp(&ovrStart, &ovrEnd);
                        Q_ASSERT(overHead <= 0);
                    }
                    if (stop_)
                        ret = -2;
                }
                else
                {
                    ret = sendQueueTransmit(handle_, seq->sendQueue_,
                            overHead, kSyncTransmit);
                }
#else
                ret = sendQueueTransmit(handle_, seq->sendQueue_,
                            overHead, kSyncTransmit);
#endif

                if (ret >= 0)
                {
                    long usecs = seq->usecDelay_ + overHead;
                    if (usecs > 0)
                    {
                        (*udelayFn_)(usecs);
                        overHead = 0;
                    }
                    else
                        overHead = usecs;
                }
                else
                {
                    qDebug("error %d in sendQueueTransmit()", ret);
                    qDebug("overHead = %ld", overHead);
                    stop_ = false;
                    goto _exit;
                }
            }
        }

        // Move to the next Packet Set
        i += rptSz;
    }

    if (returnToQIdx_ >= 0)
    {
        long usecs = loopDelay_ + overHead;

        if (usecs > 0)
        {
            (*udelayFn_)(usecs);
            overHead = 0;
        }
        else
            overHead = usecs;

        i = returnToQIdx_;
        goto _restart;
    }

_exit:
    if (trackStreamStats_)
        updateStreamStats();

    state_ = kFinished;
}

void PcapTxThread::start()
{
    // FIXME: return error
    if (state_ == kRunning) {
        qWarning("Transmit start requested but is already running!");
        return;
    }

    state_ = kNotStarted;
    QThread::start();

    while (state_ == kNotStarted)
        QThread::msleep(10);
}

void PcapTxThread::stop()
{
    if (state_ == kRunning) {
        stop_ = true;
        while (state_ == kRunning)
            QThread::msleep(10);
    }
    else {
        // FIXME: return error
        qWarning("Transmit stop requested but is not running!");
        return;
    }
}

bool PcapTxThread::isRunning()
{
    return (state_ == kRunning);
}

int PcapTxThread::sendQueueTransmit(pcap_t *p,
        pcap_send_queue *queue, long &overHead, int sync)
{
    TimeStamp ovrStart, ovrEnd;
    struct timeval ts;
    struct pcap_pkthdr *hdr = (struct pcap_pkthdr*) queue->buffer;
    char *end = queue->buffer + queue->len;

    ts = hdr->ts;

    getTimeStamp(&ovrStart);
    while((char*) hdr < end)
    {
        uchar *pkt = (uchar*)hdr + sizeof(*hdr);
        int pktLen = hdr->caplen;

        if (sync)
        {
            long usec = (hdr->ts.tv_sec - ts.tv_sec) * 1000000 +
                (hdr->ts.tv_usec - ts.tv_usec);

            getTimeStamp(&ovrEnd);

            overHead -= udiffTimeStamp(&ovrStart, &ovrEnd);
            Q_ASSERT(overHead <= 0);
            usec += overHead;
            if (usec > 0)
            {
                (*udelayFn_)(usec);
                overHead = 0;
            }
            else
                overHead = usec;

            ts = hdr->ts;
            getTimeStamp(&ovrStart);
        }

        Q_ASSERT(pktLen > 0);

        pcap_sendpacket(p, pkt, pktLen);
        stats_->pkts++;
        stats_->bytes += pktLen;

        // Step to the next packet in the buffer
        hdr = (struct pcap_pkthdr*) (pkt + pktLen);
        pkt = (uchar*) ((uchar*)hdr + sizeof(*hdr)); // FIXME: superfluous?

        if (stop_)
        {
            return -2;
        }
    }

    return 0;
}

void PcapTxThread::updateStreamStats()
{
    // If no packets in list, nothing to be done
    if (!packetListSize_)
        return;

    // Get number of tx packets sent during last transmit
    quint64 pkts = stats_->pkts > lastStats_.pkts ?
        stats_->pkts - lastStats_.pkts :
        stats_->pkts + (ULLONG_MAX - lastStats_.pkts);

    // Calculate -
    //   number of complete repeats of packetList_
    //      => each PacketSet in the packetList is repeated these many times
    //   number of pkts sent in last partial repeat of packetList_
    //      - This encompasses 0 or more potentially partial PacketSets
    // XXX: Note for the above, we consider a PacketSet to include its
    // own repeats within itself
    int c = pkts/packetListSize_;
    int d = pkts%packetListSize_;

    qDebug("%s:", __FUNCTION__);
    qDebug("txPkts = %llu", pkts);
    qDebug("packetListSize_ = %llu", packetListSize_);
    qDebug("c = %d, d = %d\n", c, d);

    int i;

    if (!c)
        goto _last_repeat;

    i = 0;
    while (i < packetSequenceList_.size()) {
        PacketSequence *seq = packetSequenceList_.at(i);
        int rptSz = seq->repeatSize_;
        int rptCnt = seq->repeatCount_;

        for (int k = 0; k < rptSz; k++) {
            seq = packetSequenceList_.at(i+k);
            StreamStatsIterator iter(seq->streamStatsMeta_);
            while (iter.hasNext()) {
                iter.next();
                uint guid = iter.key();
                StreamStatsTuple ssm = iter.value();
                streamStats_[guid].tx_pkts += c * rptCnt * ssm.tx_pkts;
                streamStats_[guid].tx_bytes += c * rptCnt * ssm.tx_bytes;
            }
        }
        // Move to the next Packet Set
        i += rptSz;
    }

_last_repeat:
    if (!d)
        goto _done;

    i = 0;
    while (i < packetSequenceList_.size()) {
        PacketSequence *seq = packetSequenceList_.at(i);
        int rptSz = seq->repeatSize_;
        int rptCnt = seq->repeatCount_;

        for (int j = 0; j < rptCnt; j++) {
            for (int k = 0; k < rptSz; k++) {
                seq = packetSequenceList_.at(i+k);
                Q_ASSERT(seq->packets_);
                if (d >= seq->packets_) {
                    // All packets of this seq were sent
                    StreamStatsIterator iter(seq->streamStatsMeta_);
                    while (iter.hasNext()) {
                        iter.next();
                        uint guid = iter.key();
                        StreamStatsTuple ssm = iter.value();
                        streamStats_[guid].tx_pkts += ssm.tx_pkts;
                        streamStats_[guid].tx_bytes += ssm.tx_bytes;
                    }
                    d -= seq->packets_;
                }
                else { // (d < seq->packets_)
                    // not all packets of this seq were sent, so we need to
                    // traverse this seq upto 'd' pkts, parse guid from the
                    // packet and update streamStats
                    struct pcap_pkthdr *hdr =
                        (struct pcap_pkthdr*) seq->sendQueue_->buffer;
                    char *end = seq->sendQueue_->buffer + seq->sendQueue_->len;

                    while(d && ((char*) hdr < end)) {
                        uchar *pkt = (uchar*)hdr + sizeof(*hdr);
                        uint guid;

                        if (SignProtocol::packetGuid(pkt, hdr->caplen, &guid)) {
                            streamStats_[guid].tx_pkts++;
                            streamStats_[guid].tx_bytes += hdr->caplen;
                        }

                        // Step to the next packet in the buffer
                        hdr = (struct pcap_pkthdr*) (pkt + hdr->caplen);
                        d--;
                    }
                    Q_ASSERT(d == 0);
                    goto _done;
                }
            }
        }
        // Move to the next Packet Set
        i += rptSz;
    }

_done:
    return;
}

void PcapTxThread::udelay(unsigned long usec)
{
#if defined(Q_OS_WIN32)
    LARGE_INTEGER tgtTicks;
    LARGE_INTEGER curTicks;

    QueryPerformanceCounter(&curTicks);
    tgtTicks.QuadPart = curTicks.QuadPart + (usec*gTicksFreq)/1000000;

    while (curTicks.QuadPart < tgtTicks.QuadPart)
        QueryPerformanceCounter(&curTicks);
#elif defined(Q_OS_LINUX)
    struct timeval delay, target, now;

    //qDebug("usec delay = %ld", usec);

    delay.tv_sec = 0;
    delay.tv_usec = usec;

    while (delay.tv_usec >= 1000000)
    {
        delay.tv_sec++;
        delay.tv_usec -= 1000000;
    }

    gettimeofday(&now, NULL);
    timeradd(&now, &delay, &target);

    do {
        gettimeofday(&now, NULL);
    } while (timercmp(&now, &target, <));
#else
    QThread::usleep(usec);
#endif
}

