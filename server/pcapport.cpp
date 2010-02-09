#include "pcapport.h"

#include <QtGlobal>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

pcap_if_t *PcapPort::deviceList_ = NULL;

PcapPort::PcapPort(int id, const char *device)
    : AbstractPort(id, device)
{
    monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
    monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);
    transmitter_ = new PortTransmitter(device);
    capturer_ = new PortCapturer(device);

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
    delete capturer_;
    delete transmitter_;
    delete monitorTx_;
    delete monitorRx_;
}

void PcapPort::updateNotes()
{
    QString notes;

    if (!monitorRx_->isDirectional() && !hasExclusiveControl())
        notes.append("<i>Rx Frames/Bytes</i>: Includes non Ostinato Tx pkts also (Tx by Ostinato are not included)<br>");

    if (!monitorTx_->isDirectional() && !hasExclusiveControl())
        notes.append("<i>Tx Frames/Bytes</i>: Only Ostinato Tx pkts (Tx by others NOT included)<br>");

    if (notes.isEmpty())
        data_.set_notes("");
    else
        data_.set_notes(QString("<b>Limitation(s)</b>"
            "<p>%1<br>"
            "Rx/Tx Rates are also subject to above limitation(s)</p>").
            arg(notes).toStdString());
}

PcapPort::PortMonitor::PortMonitor(const char *device, Direction direction,
        AbstractPort::PortStats *stats)
{
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE];

    direction_ = direction;
    isDirectional_ = true;
    stats_ = stats;
    handle_ = pcap_open_live(device, 64 /* FIXME */, PCAP_OPENFLAG_PROMISCUOUS,
            1000 /* ms */, errbuf);

    if (handle_ == NULL)
        goto _open_error;

    switch (direction_)
    {
    case kDirectionRx:
        ret = pcap_setdirection(handle_, PCAP_D_IN);
        break;
    case kDirectionTx:
        ret = pcap_setdirection(handle_, PCAP_D_OUT);
        break;
    default:
        Q_ASSERT(false);
    }

    if (ret < 0)
        goto _set_direction_error;

    return;

_set_direction_error:
    qDebug("Error setting direction(%d) %s: %s\n", direction, device, 
            pcap_geterr(handle_));
    isDirectional_ = false;
    return;

_open_error:
    qDebug("Error opening port %s: %s\n", device, pcap_geterr(handle_));
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
    char errbuf[PCAP_ERRBUF_SIZE];

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
    handle_ = pcap_open_live(device, 64 /* FIXME */, PCAP_OPENFLAG_PROMISCUOUS,
            1000 /* ms */, errbuf);

    if (handle_ == NULL)
        goto _open_error;

    usingInternalHandle_ = true;

    return;

_open_error:
    qDebug("Error opening port %s: %s\n", device, pcap_geterr(handle_));
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
    setPacketListLoopMode(false, 0); 
}

bool PcapPort::PortTransmitter::appendToPacketList(long sec, long usec, 
        const uchar *packet, int length)
{
    bool op = true;
    pcap_pkthdr pktHdr;
    pcap_send_queue *sendQ;

    pktHdr.caplen = pktHdr.len = length;
    pktHdr.ts.tv_sec = sec;
    pktHdr.ts.tv_usec = usec;

    sendQ = sendQueueList_.isEmpty() ? NULL : sendQueueList_.last();
 
    if ((sendQ == NULL) || 
            (sendQ->len + sizeof(pcap_pkthdr) + length) > sendQ->maxlen)
    {
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

    qDebug("sendQueueList_.size = %d", sendQueueList_.size());

    for(i = 0; i < sendQueueList_.size(); i++)
    {
        int ret;
_restart:
        ret = sendQueueTransmit(handle_, sendQueueList_.at(i), kSyncTransmit);

        if (ret < 0)
        {
            qDebug("error in sendQueueTransmit()");
            return;
        }
    }

    if (returnToQIdx_ >= 0)
    {
        i = returnToQIdx_;

        udelay(loopDelay_);
        goto _restart;
    }
}

void PcapPort::PortTransmitter::stop()
{
    stop_ = true;
}

int PcapPort::PortTransmitter::sendQueueTransmit(pcap_t *p,
        pcap_send_queue *queue, int sync)
{
    struct timeval ts;
    struct pcap_pkthdr *hdr = (struct pcap_pkthdr*) queue->buffer;
    char *end = queue->buffer + queue->len;

    if (sync)
        ts = hdr->ts;

    while (1)
    {
        uchar *pkt = (uchar*)hdr + sizeof(*hdr);
        int pktLen = hdr->caplen;

        if (stop_)
        {
            stop_ = false;
            return -2;
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
        hdr = (struct pcap_pkthdr*) ((uchar*)hdr + sizeof(*hdr) + pktLen);
        pkt = (uchar*) ((uchar*)hdr + sizeof(*hdr));

        // Check if the end of the user buffer has been reached
        if((char*) hdr >= end)
            return 0;

        if (sync)
        {
            long usec = (hdr->ts.tv_sec - ts.tv_sec) * 1000000 +
                (hdr->ts.tv_usec - ts.tv_usec);

            if (usec)
            {
                udelay(usec);
                ts = hdr->ts;
            }
        }
    }
}

void PcapPort::PortTransmitter::udelay(long usec)
{
#ifdef Q_OS_WIN32
    LARGE_INTEGER tgtTicks;
    LARGE_INTEGER curTicks;

    QueryPerformanceCounter(&curTicks);
    tgtTicks.QuadPart = curTicks.QuadPart + (usec*ticksFreq_)/1000000;

    while (curTicks.QuadPart < tgtTicks.QuadPart)
        QueryPerformanceCounter(&curTicks);
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
    char errbuf[PCAP_ERRBUF_SIZE];
    
    qDebug("In %s", __PRETTY_FUNCTION__);

    if (!capFile_.isOpen())
    {
        qWarning("temp cap file is not open");
        return;
    }

    handle_ = pcap_open_live(device_.toAscii().constData(), 65535, 
                    PCAP_OPENFLAG_PROMISCUOUS, 1000 /* ms */, errbuf);
    if (handle_ == NULL)
    {
        qDebug("Error opening port %s: %s\n", 
                device_.toAscii().constData(), pcap_geterr(handle_));
        return;
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
            stop_ = false;
            break;
        }
    }
    pcap_dump_close(dumpHandle_);
    pcap_close(handle_);
    dumpHandle_ = NULL;
    handle_ = NULL;
}

void PcapPort::PortCapturer::stop()
{
    stop_ = true;
}

QFile* PcapPort::PortCapturer::captureFile()
{
    return &capFile_;
}
