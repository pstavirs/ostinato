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

#include "devicemanager.h"
#include "packetbuffer.h"

#include <QtGlobal>

pcap_if_t *PcapPort::deviceList_ = NULL;

PcapPort::PcapPort(int id, const char *device)
    : AbstractPort(id, device)
{
    monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
    monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);
    transmitter_ = new PcapTransmitter(device, streamStats_);
    capturer_ = new PortCapturer(device);
    emulXcvr_ = new EmulationTransceiver(device, deviceManager_);
    rxStatsPoller_ = new PcapRxStats(device, streamStats_);

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
            if (dev->name)
                data_.set_name(dev->name);
            if (dev->description)
                data_.set_description(dev->description);

            //! \todo set port IP addr also
        }
    }
}

void PcapPort::init()
{
    AbstractPort::init();

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

    if (monitorRx_)
        monitorRx_->stop();
    if (monitorTx_)
        monitorTx_->stop();

    rxStatsPoller_->stop();
    delete rxStatsPoller_;

    delete emulXcvr_;
    delete capturer_;
    delete transmitter_;

    if (monitorRx_)
        monitorRx_->wait();
    delete monitorRx_;

    if (monitorTx_)
        monitorTx_->wait();
    delete monitorTx_;
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

bool PcapPort::setTrackStreamStats(bool enable)
{
    bool val = enable ? startStreamStatsTracking() : stopStreamStatsTracking();

    if (val)
        AbstractPort::setTrackStreamStats(enable);

    return val;
}

bool PcapPort::setRateAccuracy(AbstractPort::Accuracy accuracy)
{
    if (transmitter_->setRateAccuracy(accuracy)) {
        AbstractPort::setRateAccuracy(accuracy);
        return true;
    }
    return false;
}

void PcapPort::startDeviceEmulation()
{
    emulXcvr_->start();
}

void PcapPort::stopDeviceEmulation()
{
    emulXcvr_->stop();
}

int PcapPort::sendEmulationPacket(PacketBuffer *pktBuf)
{
    return emulXcvr_->transmitPacket(pktBuf);
}

bool PcapPort::startStreamStatsTracking()
{
    if (!transmitter_->setStreamStatsTracking(true))
        goto _tx_fail;
    if (!rxStatsPoller_->start())
        goto _rx_fail;
    /*
     * If RxPoller receives both IN and OUT packets, packets Tx on this
     * port will also be received by it and we consider it to be a Rx (IN)
     * packet incorrectly - so adjust Rx stats for this case
     * XXX - ideally, RxPoller should do this adjustment, but given our
     * design, it is easier to implement in transmitter
     */
    transmitter_->adjustRxStreamStats(!rxStatsPoller_->isDirectional());
    return true;

_rx_fail:
    transmitter_->setStreamStatsTracking(false);
_tx_fail:
    qWarning("failed to start stream stats tracking");
    return false;
}

bool PcapPort::stopStreamStatsTracking()
{
    if (!transmitter_->setStreamStatsTracking(false))
        goto _tx_fail;
    if (!rxStatsPoller_->stop())
        goto _rx_fail;
    return true;

_rx_fail:
    transmitter_->setStreamStatsTracking(true);
_tx_fail:
    qWarning("failed to stop stream stats tracking");
    return false;
}

/*
 * ------------------------------------------------------------------- *
 * Port Monitor
 * ------------------------------------------------------------------- *
 */
PcapPort::PortMonitor::PortMonitor(const char *device, Direction direction,
        AbstractPort::PortStats *stats)
{
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE] = "";
    bool noLocalCapture;

    direction_ = direction;
    isDirectional_ = true;
    isPromisc_ = true;
    noLocalCapture = true;
    stats_ = stats;
    stop_ = false;

_retry:
#ifdef Q_OS_WIN32
    int flags = 0;

    if (isPromisc_)
        flags |= PCAP_OPENFLAG_PROMISCUOUS;
    if (noLocalCapture)
        flags |= PCAP_OPENFLAG_NOCAPTURE_LOCAL;

    handle_ = pcap_open(device, 64 /* FIXME */, flags,
                1000 /* ms */, NULL, errbuf);
#else
    handle_ = pcap_open_live(device, 64 /* FIXME */, int(isPromisc_),
                1000 /* ms */, errbuf);
#endif

    if (handle_ == NULL)
    {
        if (isPromisc_ && QString(errbuf).contains("promiscuous"))
        {
            qDebug("Can't set promiscuous mode, trying non-promisc %s", device);
            isPromisc_ = false;
            goto _retry;
        }
        else if (noLocalCapture && QString(errbuf).contains("loopback"))
        {
            qDebug("Can't set no local capture mode %s", device);
            noLocalCapture = false;
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
    if (handle_)
        pcap_close(handle_);
}

void PcapPort::PortMonitor::run()
{
    while (!stop_)
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
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle_));
                break;
            default:
                qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
        }
    }
}

void PcapPort::PortMonitor::stop()
{
    stop_ = true;
    pcap_breakloop(handle());
}

/*
 * ------------------------------------------------------------------- *
 * Port Capturer
 * ------------------------------------------------------------------- *
 */
PcapPort::PortCapturer::PortCapturer(const char *device)
{
    device_ = QString::fromLatin1(device);
    stop_ = false;
    state_ = kNotStarted;

    if (!capFile_.open())
        qWarning("Unable to open temp cap file");

    qDebug("cap file = %s", qPrintable(capFile_.fileName()));

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
        goto _exit;
    }
_retry:
    handle_ = pcap_open_live(qPrintable(device_), 65535,
                    flag, 1000 /* ms */, errbuf);

    if (handle_ == NULL)
    {
        if (flag && QString(errbuf).contains("promiscuous"))
        {
            qDebug("%s:can't set promiscuous mode, trying non-promisc", 
                    qPrintable(device_));
            flag = 0;
            goto _retry;
        }
        else
        {
            qDebug("%s: Error opening port %s: %s\n", __FUNCTION__,
                    qPrintable(device_), errbuf);
            goto _exit;
        }
    }

    dumpHandle_ = pcap_dump_open(handle_, qPrintable(capFile_.fileName()));
    PcapSession::preRun();
    state_ = kRunning;
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
                qDebug("Loop/signal break or some other error");
                break;
            default:
                qWarning("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
                stop_ = true;
        }

        if (stop_) 
        {
            qDebug("user requested capture stop");
            break;
        }
    }
    PcapSession::postRun();

    pcap_dump_close(dumpHandle_);
    pcap_close(handle_);
    dumpHandle_ = NULL;
    handle_ = NULL;
    stop_ = false;

_exit:
    state_ = kFinished;
}

void PcapPort::PortCapturer::start()
{
    // FIXME: return error
    if (state_ == kRunning) {
        qWarning("Capture start requested but is already running!");
        return;
    }

    state_ = kNotStarted;
    QThread::start();

    while (state_ == kNotStarted)
        QThread::msleep(10);
}

void PcapPort::PortCapturer::stop()
{
    if (state_ == kRunning) {
        stop_ = true;
        PcapSession::stop(handle_);
        while (state_ == kRunning)
            QThread::msleep(10);
    }
    else {
        // FIXME: return error
        qWarning("Capture stop requested but is not running!");
        return;
    }
}

bool PcapPort::PortCapturer::isRunning()
{
    return (state_ == kRunning);
}

QFile* PcapPort::PortCapturer::captureFile()
{
    return &capFile_;
}


/*
 * ------------------------------------------------------------------- *
 * Transmit+Receiver for Device/ProtocolEmulation
 * ------------------------------------------------------------------- *
 */
PcapPort::EmulationTransceiver::EmulationTransceiver(const char *device,
        DeviceManager *deviceManager)
{
    device_ = QString::fromLatin1(device);
    deviceManager_ = deviceManager;
    stop_ = false;
    state_ = kNotStarted;
    handle_ = NULL;
}

PcapPort::EmulationTransceiver::~EmulationTransceiver()
{
    stop();
}

void PcapPort::EmulationTransceiver::run()
{
    int flags = PCAP_OPENFLAG_PROMISCUOUS;
    char errbuf[PCAP_ERRBUF_SIZE] = "";
    struct bpf_program bpf;
#if 0
    const char *capture_filter =
        "arp or icmp or icmp6 or "
        "(vlan and (arp or icmp or icmp6)) or "
        "(vlan and vlan and (arp or icmp or icmp6)) or "
        "(vlan and vlan and vlan and (arp or icmp or icmp6)) or "
        "(vlan and vlan and vlan and vlan and (arp or icmp or icmp6))";
/*
    Ideally we should use the above filter, but the 'vlan' capture filter
    in libpcap is implemented as a kludge. From the pcap-filter man page -

    vlan [vlan_id]
       Note that the first vlan keyword encountered in expression changes
       the decoding offsets for the remainder of expression on the
       assumption that the packet is a VLAN packet.

       The  vlan [vlan_id] expression may be used more than once, to filter on
       VLAN hierarchies. Each use of that expression increments the filter
       offsets by 4.

    See https://ask.wireshark.org/questions/31953/unusual-behavior-with-stacked-vlan-tags-and-capture-filter

    So we use the modified filter expression that works as we intend. If ever
    libpcap changes their implementation, this will need to change as well.
*/
#else
    const char *capture_filter =
        "arp or icmp or icmp6 or "
        "(vlan and (arp or icmp or icmp6)) or "
        "(vlan and (arp or icmp or icmp6)) or "
        "(vlan and (arp or icmp or icmp6)) or "
        "(vlan and (arp or icmp or icmp6))";
#endif

    const int optimize = 1;

    qDebug("In %s", __PRETTY_FUNCTION__);

#ifdef Q_OS_WIN32
    flags |= PCAP_OPENFLAG_NOCAPTURE_LOCAL;
#endif

#ifdef Q_OS_WIN32
_retry:
    // NOCAPTURE_LOCAL needs windows only pcap_open()
    handle_ = pcap_open(qPrintable(device_), 65535,
                flags, 100 /* ms */, NULL, errbuf);
#else
    handle_ = pcap_open_live(qPrintable(device_), 65535,
                    flags, 100 /* ms */, errbuf);
#endif

    if (handle_ == NULL)
    {
        if (flags && QString(errbuf).contains("promiscuous"))
        {
            Xnotify("Unable to set promiscuous mode on <%s> - "
                    "device emulation will not work", qPrintable(device_));
            goto _exit;
        }
#ifdef Q_OS_WIN32
        else if ((flags & PCAP_OPENFLAG_NOCAPTURE_LOCAL)
                && QString(errbuf).contains("loopback"))
        {
            qDebug("Can't set no local capture mode %s", qPrintable(device_));
            flags &= ~PCAP_OPENFLAG_NOCAPTURE_LOCAL;
            goto _retry;
        }
#endif
        else
        {
            Xnotify("Unable to open <%s> [%s] - device emulation will not work",
                    qPrintable(device_), errbuf);
            goto _exit;
        }
    }

    // TODO: for now the filter is hardcoded to accept tagged/untagged
    // ARP/NDP or ICMPv4/v6; when more protocols are added, we may need
    // to derive this filter based on which protocols are configured
    // on the devices
    if (pcap_compile(handle_, &bpf, capture_filter, optimize, 0) < 0)
    {
        qWarning("%s: error compiling filter: %s", qPrintable(device_),
                pcap_geterr(handle_));
        goto _skip_filter;
    }

    if (pcap_setfilter(handle_, &bpf) < 0)
    {
        qWarning("%s: error setting filter: %s", qPrintable(device_),
                pcap_geterr(handle_));
        goto _skip_filter;
    }

_skip_filter:
    PcapSession::preRun();
    state_ = kRunning;
    while (1)
    {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle_, &hdr, &data);
        switch (ret)
        {
            case 1:
            {
                PacketBuffer *pktBuf = new PacketBuffer(data, hdr->caplen);
#if 0
                for (int i = 0; i < 64; i++) {
                    printf("%02x ", data[i]);
                    if (i % 16 == 0)
                        printf("\n");
                }
                printf("\n");
#endif
                // XXX: deviceManager should free pktBuf before returning
                // from this call; if it needs to process the pkt async
                // it should make a copy as the pktBuf's data buffer is
                // owned by libpcap which does not guarantee data will
                // persist across calls to pcap_next_ex()
                deviceManager_->receivePacket(pktBuf);
                break;
            }
            case 0:
                // timeout: just go back to the loop
                break;
            case -1:
                qWarning("%s: error reading packet (%d): %s",
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle_));
                break;
            case -2:
                qDebug("Loop/signal break or some other error");
                break;
            default:
                qWarning("%s: Unexpected return value %d", __PRETTY_FUNCTION__,
                        ret);
                stop_ = true;
        }

        if (stop_)
        {
            qDebug("user requested receiver stop");
            break;
        }
    }
    PcapSession::postRun();

    pcap_close(handle_);
    handle_ = NULL;
    stop_ = false;

_exit:
    state_ = kFinished;
}

void PcapPort::EmulationTransceiver::start()
{
    if (state_ == kRunning) {
        qWarning("Receive start requested but is already running!");
        return;
    }

    state_ = kNotStarted;
    QThread::start();

    while (state_ == kNotStarted)
        QThread::msleep(10);
}

void PcapPort::EmulationTransceiver::stop()
{
    if (state_ == kRunning) {
        stop_ = true;
        PcapSession::stop(handle_);
        while (state_ == kRunning)
            QThread::msleep(10);
    }
    else {
        qWarning("Receive stop requested but is not running!");
        return;
    }
}

bool PcapPort::EmulationTransceiver::isRunning()
{
    return (state_ == kRunning);
}

int PcapPort::EmulationTransceiver::transmitPacket(PacketBuffer *pktBuf)
{
    return pcap_sendpacket(handle_, pktBuf->data(), pktBuf->length());
}
