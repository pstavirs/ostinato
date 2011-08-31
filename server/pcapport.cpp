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

#include "pcapport.h"

#include <QtGlobal>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

pcap_if_t *PcapPort::deviceList_ = NULL;


#if defined(Q_OS_LINUX)
typedef struct timeval TimeStamp;
static void inline getTimeStamp(TimeStamp *stamp)
{
    gettimeofday(stamp, NULL);
}

// Returns time diff in usecs between end and start
static long inline udiffTimeStamp(const TimeStamp *start, const TimeStamp *end)
{
    struct timeval diff;
    long usecs;

    timersub(end, start, &diff);

    usecs = diff.tv_usec;
    if (diff.tv_sec)
        usecs += diff.tv_sec*1e6;

    return usecs;
}
#else
typedef int TimeStamp;
static void inline getTimeStamp(TimeStamp*) {}
static long inline udiffTimeStamp(const TimeStamp*, const TimeStamp*) { return 0; }
#endif

PcapPort::PcapPort(int id, const char *device)
    : AbstractPort(id, device)
{
    monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
    monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);
    transmitter_ = new PortTransmitter(device);
    capturer_ = new PortCapturer(device);

    if (!monitorRx_->handle() || !monitorTx_->handle())
        isUsable_ = false;

    if (!deviceList_)
    {
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs(&deviceList_, errbuf) == -1)
            qDebug("Error in pcap_findalldevs_ex: %s\n", errbuf);
    }

    for (pcap_if_t *dev = deviceList_; dev != NULL; dev = dev->next)
    {
        if (strcmp(device, dev->name) == 0)
        {
#ifdef Q_OS_WIN32
            data_.set_name(QString("if%1 ").arg(id).toStdString());
#else
            if (dev->name)
                data_.set_name(dev->name);
#endif
            if (dev->description)
                data_.set_description(dev->description);

            //! \todo set port IP addr also
        }
    }
}

void PcapPort::init()
{
    if (!monitorTx_->isDirectional())
        transmitter_->useExternalStats(&stats_);

    transmitter_->setHandle(monitorRx_->handle());

    updateNotes();

    monitorRx_->start();
    monitorTx_->start();
}

PcapPort::~PcapPort()
{
    qDebug("In %s", __FUNCTION__);
    delete capturer_;
    delete transmitter_;
    delete monitorTx_;
    delete monitorRx_;
}

void PcapPort::updateNotes()
{
    QString notes;

    if ((!monitorRx_->isPromiscuous()) || (!monitorTx_->isPromiscuous()))
        notes.append("<li>Non Promiscuous Mode</li>");

    if (!monitorRx_->isDirectional() && !hasExclusiveControl())
        notes.append("<li><i>Rx Frames/Bytes</i>: Includes non Ostinato Tx pkts also (Tx by Ostinato are not included)</li>");

    if (!monitorTx_->isDirectional() && !hasExclusiveControl())
        notes.append("<li><i>Tx Frames/Bytes</i>: Only Ostinato Tx pkts (Tx by others NOT included)</li>");

    if (notes.isEmpty())
        data_.set_notes("");
    else
        data_.set_notes(QString("<b>Limitation(s)</b>"
            "<ul>%1</ul>"
            "Rx/Tx Rates are also subject to above limitation(s)").
            arg(notes).toStdString());
}

PcapPort::PortMonitor::PortMonitor(const char *device, Direction direction,
        AbstractPort::PortStats *stats)
{
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE] = "";

    direction_ = direction;
    isDirectional_ = true;
    isPromisc_ = true;
    stats_ = stats;
_retry:
    handle_ = pcap_open_live(device, 64 /* FIXME */, int(isPromisc_),
            1000 /* ms */, errbuf);

    if (handle_ == NULL)
    {
        if (isPromisc_ && QString(errbuf).contains("promiscuous"))
        {
            qDebug("%s:can't set promiscuous mode, trying non-promisc", device);
            isPromisc_ = false;
            goto _retry;
        }
        else
            goto _open_error;
    }
#ifdef Q_OS_WIN32
    // pcap_setdirection() API is not supported in Windows.
    // NOTE: WinPcap 4.1.1 and above exports a dummy API that returns -1
    // but since we would like to work with previous versions of WinPcap
    // also, we assume the API does not exist
    ret = -1;
#else
    switch (direction_)
    {
    case kDirectionRx:
        ret = pcap_setdirection(handle_, PCAP_D_IN);
        break;
    case kDirectionTx:
        ret = pcap_setdirection(handle_, PCAP_D_OUT);
        break;
    default:
        ret = -1; // avoid 'may be used uninitialized' warning
        Q_ASSERT(false);
    }
#endif

    if (ret < 0)
        goto _set_direction_error;

    return;

_set_direction_error:
    qDebug("Error setting direction(%d) %s: %s\n", direction, device, 
            pcap_geterr(handle_));
    isDirectional_ = false;
    return;

_open_error:
    qDebug("%s: Error opening port %s: %s\n", __FUNCTION__, device, errbuf);
} 

PcapPort::PortMonitor::~PortMonitor()
{
    pcap_close(handle_);
}

void PcapPort::PortMonitor::run()
{
    while (1)
    {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle_, &hdr, &data);
        switch (ret)
        {
            case 1:
                switch (direction_)
                {
                case kDirectionRx:
                    stats_->rxPkts++;
                    stats_->rxBytes += hdr->len;
                    break;

                case kDirectionTx:
                    if (isDirectional_)
                    {
                        stats_->txPkts++;
                        stats_->txBytes += hdr->len;
                    }
                    break;

                default:
                    Q_ASSERT(false);
                }

                //! \todo TODO pkt/bit rates
                break;
            case 0:
                //qDebug("%s: timeout. continuing ...", __PRETTY_FUNCTION__);
                continue;
            case -1:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle_));
                break;
            case -2:
            default:
                qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
        }
    }
}

PcapPort::PortTransmitter::PortTransmitter(const char *device)
{
    char errbuf[PCAP_ERRBUF_SIZE] = "";

#ifdef Q_OS_WIN32
    LARGE_INTEGER   freq;
    if (QueryPerformanceFrequency(&freq))
        ticksFreq_ = freq.QuadPart;
    else
        Q_ASSERT_X(false, "PortTransmitter::PortTransmitter",
                "This Win32 platform does not support performance counter");
#endif
    returnToQIdx_ = -1;
    loopDelay_ = 0;
    stop_ = false;
    stats_ = new AbstractPort::PortStats;
    usingInternalStats_ = true;
    handle_ = pcap_open_live(device, 64 /* FIXME */, 0, 1000 /* ms */, errbuf);

    if (handle_ == NULL)
        goto _open_error;

    usingInternalHandle_ = true;

    return;

_open_error:
    qDebug("%s: Error opening port %s: %s\n", __FUNCTION__, device, errbuf);
    usingInternalHandle_ = false;
}

PcapPort::PortTransmitter::~PortTransmitter()
{
    if (usingInternalStats_)
        delete stats_;
}

void PcapPort::PortTransmitter::clearPacketList()
{
    Q_ASSERT(!isRunning());
    // \todo lock for sendQueueList
    while(sendQueueList_.size())
    {
        pcap_send_queue *sq = sendQueueList_.takeFirst();
        pcap_sendqueue_destroy(sq);
    }
    setPacketListLoopMode(false, 0, 0); 
}

bool PcapPort::PortTransmitter::appendToPacketList(long sec, long nsec, 
        const uchar *packet, int length)
{
    bool op = true;
    pcap_pkthdr pktHdr;
    pcap_send_queue *sendQ;

    pktHdr.caplen = pktHdr.len = length;
    pktHdr.ts.tv_sec = sec;
    pktHdr.ts.tv_usec = nsec/1000;

    sendQ = sendQueueList_.isEmpty() ? NULL : sendQueueList_.last();
 
    if ((sendQ == NULL) || 
            (sendQ->len + 2*sizeof(pcap_pkthdr) + length) > sendQ->maxlen)
    {
        // Add a zero len packet at end of sendQ for inter-sendQ timing
        if (sendQ)
        {
            pcap_pkthdr hdr = pktHdr;
            hdr.caplen = 0;
            pcap_sendqueue_queue(sendQ, &hdr, (u_char*)packet);
        }
        //! \todo (LOW): calculate sendqueue size
        sendQ = pcap_sendqueue_alloc(1*1024*1024);
        sendQueueList_.append(sendQ);

        // Validate that the pkt will fit inside the new sendQ
        Q_ASSERT((length + sizeof(pcap_pkthdr)) < sendQ->maxlen);
    }

    if (pcap_sendqueue_queue(sendQ, &pktHdr, (u_char*) packet) < 0)
        op = false;

    return op;
}

void PcapPort::PortTransmitter::setHandle(pcap_t *handle)
{
    if (usingInternalHandle_)
        pcap_close(handle_);
    handle_ = handle;
    usingInternalStats_ = false;
}

void PcapPort::PortTransmitter::useExternalStats(AbstractPort::PortStats *stats)
{
    if (usingInternalStats_)
        delete stats_;
    stats_ = stats;
    usingInternalStats_ = false;
}

void PcapPort::PortTransmitter::run()
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
    long overHead = 0;

    qDebug("sendQueueList_.size = %d", sendQueueList_.size());
    if (sendQueueList_.size() <= 0)
        return;

    for(i = 0; i < sendQueueList_.size(); i++)
    {
        int ret;
_restart:
        ret = sendQueueTransmit(handle_, sendQueueList_.at(i), overHead, 
                    kSyncTransmit);

        if (ret < 0)
        {
            qDebug("error %d in sendQueueTransmit()", ret);
            qDebug("overHead = %ld", overHead);
            stop_ = false;
            return;
        }
    }

    if (returnToQIdx_ >= 0)
    {
        long usecs = loopDelay_ + overHead;

        if (usecs > 0)
        {
            udelay(usecs);
            overHead = 0;
        }
        else
            overHead = usecs;

        i = returnToQIdx_;
        goto _restart;
    }
}

void PcapPort::PortTransmitter::stop()
{
    if (isRunning())
        stop_ = true;
}

int PcapPort::PortTransmitter::sendQueueTransmit(pcap_t *p,
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
            usec += overHead;
            if (usec > 0)
            {
                udelay(usec);
                overHead = 0;
            }
            else
                overHead = usec;

            ts = hdr->ts;
            getTimeStamp(&ovrStart);
        }

        // A pktLen of size 0 is used at the end of a sendQueue and before
        // the next sendQueue - i.e. for inter sendQueue timing
        if(pktLen > 0)
        {
            pcap_sendpacket(p, pkt, pktLen);
            stats_->txPkts++;
            stats_->txBytes += pktLen;
        }

        // Step to the next packet in the buffer
        hdr = (struct pcap_pkthdr*) (pkt + pktLen);
        pkt = (uchar*) ((uchar*)hdr + sizeof(*hdr));

        if (stop_)
        {
            return -2;
        }
    }

    return 0;
}

void PcapPort::PortTransmitter::udelay(long usec)
{
#if defined(Q_OS_WIN32)
    LARGE_INTEGER tgtTicks;
    LARGE_INTEGER curTicks;

    QueryPerformanceCounter(&curTicks);
    tgtTicks.QuadPart = curTicks.QuadPart + (usec*ticksFreq_)/1000000;

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

PcapPort::PortCapturer::PortCapturer(const char *device)
{
    device_ = QString::fromAscii(device);
    stop_ = false;

    if (!capFile_.open())
        qWarning("Unable to open temp cap file");

    qDebug("cap file = %s", capFile_.fileName().toAscii().constData());

    dumpHandle_ = NULL;
    handle_ = NULL;
}

PcapPort::PortCapturer::~PortCapturer()
{
    capFile_.close();
}

void PcapPort::PortCapturer::run()
{
    int flag = PCAP_OPENFLAG_PROMISCUOUS;
    char errbuf[PCAP_ERRBUF_SIZE] = "";
    
    qDebug("In %s", __PRETTY_FUNCTION__);

    if (!capFile_.isOpen())
    {
        qWarning("temp cap file is not open");
        return;
    }
_retry:
    handle_ = pcap_open_live(device_.toAscii().constData(), 65535, 
                    flag, 1000 /* ms */, errbuf);

    if (handle_ == NULL)
    {
        if (flag && QString(errbuf).contains("promiscuous"))
        {
            qDebug("%s:can't set promiscuous mode, trying non-promisc", 
                    device_.toAscii().constData());
            flag = 0;
            goto _retry;
        }
        else
        {
            qDebug("%s: Error opening port %s: %s\n", __FUNCTION__,
                    device_.toAscii().constData(), errbuf);
            return;
        }
    }

    dumpHandle_ = pcap_dump_open(handle_, 
            capFile_.fileName().toAscii().constData());

    while (1)
    {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle_, &hdr, &data);
        switch (ret)
        {
            case 1:
                pcap_dump((uchar*) dumpHandle_, hdr, data);
                break;
            case 0:
                // timeout: just go back to the loop
                break;
            case -1:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle_));
                break;
            case -2:
            default:
                qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
        }

        if (stop_) 
        {
            qDebug("user requested capture stop\n");
            break;
        }
    }
    pcap_dump_close(dumpHandle_);
    pcap_close(handle_);
    dumpHandle_ = NULL;
    handle_ = NULL;
    stop_ = false;
}

void PcapPort::PortCapturer::stop()
{
    if (isRunning())
        stop_ = true;
}

QFile* PcapPort::PortCapturer::captureFile()
{
    return &capFile_;
}
