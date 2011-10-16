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

#ifndef _SERVER_PCAP_PORT_H
#define _SERVER_PCAP_PORT_H

#include <QTemporaryFile>
#include <QThread>
#include <pcap.h>

#include "abstractport.h"
#include "pcapextra.h"

class PcapPort : public AbstractPort
{
public:
    PcapPort(int id, const char *device);
    ~PcapPort();

    void init();

    virtual bool hasExclusiveControl() { return false; }
    virtual bool setExclusiveControl(bool /*exclusive*/) { return false; }

    virtual void clearPacketList() { 
        transmitter_->clearPacketList();
        setPacketListLoopMode(false, 0, 0);
    }
    virtual void loopNextPacketSet(qint64 size, qint64 repeats,
            long repeatDelaySec, long repeatDelayNsec) {
        transmitter_->loopNextPacketSet(size, repeats, 
                repeatDelaySec, repeatDelayNsec);
    }
    virtual bool appendToPacketList(long sec, long nsec, const uchar *packet, 
            int length) {
        return transmitter_->appendToPacketList(sec, nsec, packet, length); 
    }
    virtual void setPacketListLoopMode(bool loop, long secDelay, long nsecDelay)
    {
        transmitter_->setPacketListLoopMode(loop, secDelay, nsecDelay);
    }

    virtual void startTransmit() { 
        Q_ASSERT(!isDirty());
        transmitter_->start(); 
    }
    virtual void stopTransmit()  { transmitter_->stop();  }
    virtual bool isTransmitOn() { return transmitter_->isRunning(); }

    virtual void startCapture() { capturer_->start(); }
    virtual void stopCapture()  { capturer_->stop(); }
    virtual bool isCaptureOn()  { return capturer_->isRunning(); }
    virtual QIODevice* captureData() { return capturer_->captureFile(); }

protected:
    enum Direction
    {
        kDirectionRx,
        kDirectionTx
    };

    class PortMonitor: public QThread
    {
    public:
        PortMonitor(const char *device, Direction direction,
                AbstractPort::PortStats *stats);
    ~PortMonitor();
        void run();
        pcap_t* handle() { return handle_; }
        Direction direction() { return direction_; }
        bool isDirectional() { return isDirectional_; }
        bool isPromiscuous() { return isPromisc_; }
    protected:
        AbstractPort::PortStats *stats_;
    private:
        pcap_t *handle_;
        Direction direction_;
        bool isDirectional_;
        bool isPromisc_;
    };

    class PortTransmitter: public QThread
    {
    public:
        PortTransmitter(const char *device);
        ~PortTransmitter();
        void clearPacketList();
        void loopNextPacketSet(qint64 size, qint64 repeats, 
            long repeatDelaySec, long repeatDelayNsec);
        bool appendToPacketList(long sec, long usec, const uchar *packet, 
            int length);
        void setPacketListLoopMode(bool loop, long secDelay, long nsecDelay) {
            returnToQIdx_ = loop ? 0 : -1;
            loopDelay_ = secDelay*long(1e6) + nsecDelay/1000;
        }
        void setHandle(pcap_t *handle);
        void useExternalStats(AbstractPort::PortStats *stats);
        void run();
        void stop();
    private:

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

        void udelay(long usec);
        int sendQueueTransmit(pcap_t *p, pcap_send_queue *queue, long &overHead,
                    int sync);

        quint64 ticksFreq_;
        QList<PacketSequence*> packetSequenceList_;
        PacketSequence *currentPacketSequence_;
        int repeatSequenceStart_;
        quint64 repeatSize_;
        quint64 packetCount_;

        int returnToQIdx_;
        long loopDelay_;

        bool usingInternalStats_;
        AbstractPort::PortStats *stats_;
        bool usingInternalHandle_;
        pcap_t *handle_;
        volatile bool stop_;
    };

    class PortCapturer: public QThread
    {
    public:
        PortCapturer(const char *device);
        ~PortCapturer();
        void run();
        void stop();
        QFile* captureFile();

    private:
        QString         device_;
        volatile bool   stop_;
        QTemporaryFile  capFile_;
        pcap_t          *handle_;
        pcap_dumper_t   *dumpHandle_;
    };

    PortMonitor     *monitorRx_;
    PortMonitor     *monitorTx_;

    void updateNotes();

private:
    PortTransmitter *transmitter_;
    PortCapturer    *capturer_;

    static pcap_if_t *deviceList_;
};

#endif
