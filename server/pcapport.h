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
#include "pcaprxstats.h"
#include "pcapsession.h"
#include "pcaptransmitter.h"

class PcapPort : public AbstractPort
{
public:
    PcapPort(int id, const char *device);
    ~PcapPort();

    void init();

    virtual bool hasExclusiveControl() { return false; }
    virtual bool setExclusiveControl(bool /*exclusive*/) { return false; }

    virtual bool setTrackStreamStats(bool enable);
    virtual bool setRateAccuracy(AbstractPort::Accuracy accuracy); 

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
    virtual void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay)
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

    virtual void startDeviceEmulation();
    virtual void stopDeviceEmulation();
    virtual int sendEmulationPacket(PacketBuffer *pktBuf);

protected:
    enum Direction
    {
        kDirectionRx,
        kDirectionTx
    };

    class PortMonitor: public QThread // TODO: inherit from PcapSession (only if required)
    {
    public:
        PortMonitor(const char *device, Direction direction,
                AbstractPort::PortStats *stats);
    ~PortMonitor();
        void run();
        void stop();
        pcap_t* handle() { return handle_; }
        Direction direction() { return direction_; }
        bool isDirectional() { return isDirectional_; }
        bool isPromiscuous() { return isPromisc_; }
    protected:
        AbstractPort::PortStats *stats_;
        bool stop_;
    private:
        pcap_t *handle_;
        Direction direction_;
        bool isDirectional_;
        bool isPromisc_;
    };

    class PortCapturer: public PcapSession
    {
    public:
        PortCapturer(const char *device);
        ~PortCapturer();
        void run();
        void start();
        void stop();
        bool isRunning();
        QFile* captureFile();

    private:
        enum State 
        {
            kNotStarted,
            kRunning,
            kFinished
        };

        QString         device_;
        volatile bool   stop_;
        QTemporaryFile  capFile_;
        pcap_t          *handle_;
        pcap_dumper_t   *dumpHandle_;
        volatile State  state_;
    };

    class EmulationTransceiver: public PcapSession
    {
    public:
        EmulationTransceiver(const char *device, DeviceManager *deviceManager);
        ~EmulationTransceiver();
        void run();
        void start();
        void stop();
        bool isRunning();
        int transmitPacket(PacketBuffer *pktBuf);

    private:
        enum State
        {
            kNotStarted,
            kRunning,
            kFinished
        };

        QString         device_;
        DeviceManager   *deviceManager_;
        volatile bool   stop_;
        pcap_t          *handle_;
        volatile State  state_;
    };

    PortMonitor     *monitorRx_;
    PortMonitor     *monitorTx_;

    void updateNotes();

private:
    bool startStreamStatsTracking();
    bool stopStreamStatsTracking();

    PcapTransmitter *transmitter_;
    PortCapturer    *capturer_;
    EmulationTransceiver *emulXcvr_;
    PcapRxStats *rxStatsPoller_;

    static pcap_if_t *deviceList_;
};

#endif
