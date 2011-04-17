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
        setPacketListLoopMode(false, 0);
    }
    virtual bool appendToPacketList(long sec, long usec, const uchar *packet, 
            int length) {
        return transmitter_->appendToPacketList(sec, usec, packet, length); 
    }
    virtual void setPacketListLoopMode(bool loop, long usecDelay) {
        transmitter_->setPacketListLoopMode(loop, usecDelay);
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
        bool appendToPacketList(long sec, long usec, const uchar *packet, 
            int length);
        void setPacketListLoopMode(bool loop, long usecDelay) {
            returnToQIdx_ = loop ? 0 : -1;
            loopDelay_ = usecDelay;
        }
        void setHandle(pcap_t *handle);
        void useExternalStats(AbstractPort::PortStats *stats);
        void run();
        void stop();
    private:
        void udelay(long usec);
        int sendQueueTransmit(pcap_t *p, pcap_send_queue *queue, int sync);

        quint64 ticksFreq_;
        QList<pcap_send_queue*> sendQueueList_;
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
