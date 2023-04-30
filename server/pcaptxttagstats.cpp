/*
Copyright (C) 2023 Srivats P.

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

#include "pcaptxttagstats.h"

#include "pcapextra.h"
#include "../common/debugdefs.h"
#include "../common/sign.h"
#include "streamtiming.h"

#define Xnotify qWarning // FIXME

PcapTxTtagStats::PcapTxTtagStats(const char *device, int id)
    : portId_(id)
{
    setObjectName(QString("TxT$:%1").arg(device));
    device_ = QString::fromLatin1(device);

    timing_ = StreamTiming::instance();
}

void PcapTxTtagStats::run()
{
    int flags = PCAP_OPENFLAG_PROMISCUOUS;
    char errbuf[PCAP_ERRBUF_SIZE] = "";
    struct bpf_program bpf;
    const int optimize = 1;
    QString capture_filter = QString(
            "(ether[len - 4:4] == 0x%1) and (ether[len - 5:1] == 0x%2)")
            .arg(SignProtocol::magic(), 0, BASE_HEX)
            .arg(SignProtocol::kTypeLenTtag, 0, BASE_HEX);

    qDebug("In %s", __PRETTY_FUNCTION__);
    qDebug("pcap-filter: %s", qPrintable(capture_filter));

    handle_ = pcap_open_live(qPrintable(device_), 65535,
                    flags, 100 /* ms */, errbuf);
    if (!handle_) {
        if (flags && QString(errbuf).contains("promiscuous")) {
            Xnotify("Unable to set promiscuous mode on <%s> - "
                    "stream stats time tracking will not work", qPrintable(device_));
            goto _exit;
        }
        else {
            Xnotify("Unable to open <%s> [%s] - stream stats rx will not work",
                    qPrintable(device_), errbuf);
            goto _exit;
        }
    }

#ifdef Q_OS_WIN32
    // pcap_setdirection() API is not supported in Windows.
    // NOTE: WinPcap 4.1.1 and above exports a dummy API that returns -1
    // but since we would like to work with previous versions of WinPcap
    // also, we assume the API does not exist
    isDirectional_ = false;
#else
    if (pcap_setdirection(handle_, PCAP_D_OUT) < 0) {
        qDebug("TxTtagStats: Error setting OUT direction %s: %s\n",
                qPrintable(device_), pcap_geterr(handle_));
        isDirectional_ = false;
    }
#endif

    if (pcap_compile(handle_, &bpf, qPrintable(capture_filter),
                     optimize, 0) < 0) {
        qWarning("%s: error compiling filter: %s", qPrintable(device_),
                pcap_geterr(handle_));
        goto _skip_filter;
    }

    if (pcap_setfilter(handle_, &bpf) < 0) {
        qWarning("%s: error setting filter: %s", qPrintable(device_),
                pcap_geterr(handle_));
        goto _skip_filter;
    }

_skip_filter:
    clearDebugStats();
    PcapSession::preRun();
    state_ = kRunning;
    while (1) {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle_, &hdr, &data);
        switch (ret) {
            case 1: {
                uint ttagId;
                uint guid;
                if (SignProtocol::packetTtagId(data, hdr->caplen,
                        &ttagId, &guid)) {
                    timing_->recordTxTime(portId_, guid, ttagId, hdr->ts);
                    timingDebug("[%d TX] %ld:%ld ttag %u guid %u", portId_,
                        hdr->ts.tv_sec, long(hdr->ts.tv_usec), ttagId, guid);
                }
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
                qDebug("%s: Loop/signal break or some other error",
                        __PRETTY_FUNCTION__);
                break;
            default:
                qWarning("%s: Unexpected return value %d",
                        __PRETTY_FUNCTION__, ret);
                stop_ = true;
        }

        if (stop_) {
            qDebug("user requested txTtagStats stop");
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

bool PcapTxTtagStats::start()
{
    if (state_ == kRunning) {
        qWarning("TxTtagStats start requested but is already running!");
        goto _exit;
    }

    state_ = kNotStarted;
    PcapSession::start();

    while (state_ == kNotStarted)
        QThread::msleep(10);
_exit:
    return true;
}

bool PcapTxTtagStats::stop()
{
    if (state_ == kRunning) {
        stop_ = true;
        PcapSession::stop();
        while (state_ == kRunning)
            QThread::msleep(10);
    }
    else
        qWarning("TxTtagStats stop requested but is not running!");

    return true;
}

bool PcapTxTtagStats::isRunning()
{
    return (state_ == kRunning);
}

bool PcapTxTtagStats::isDirectional()
{
    return isDirectional_;
}
