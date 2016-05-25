/*
Copyright (C) 2010-2012 Srivats P.

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

#ifndef _SERVER_ABSTRACT_PORT_H
#define _SERVER_ABSTRACT_PORT_H

#include <QList>
#include <QtGlobal>

#include "../common/protocol.pb.h"

class DeviceManager;
class StreamBase;
class PacketBuffer;
class QIODevice;

// TODO: send notification back to client(s)
#define notify qWarning

class AbstractPort
{
public:
    struct PortStats
    {
        quint64    rxPkts;
        quint64    rxBytes;
        quint64    rxPps;
        quint64    rxBps;

        quint64    rxDrops;
        quint64    rxErrors;
        quint64    rxFifoErrors;
        quint64    rxFrameErrors;

        quint64    txPkts;
        quint64    txBytes;
        quint64    txPps;
        quint64    txBps;
    };

    enum Accuracy
    {
        kHighAccuracy,
        kMediumAccuracy,
        kLowAccuracy,
    };

    AbstractPort(int id, const char *device);
    virtual ~AbstractPort();

    bool isUsable() { return isUsable_; }

    virtual void init();

    int id() { return data_.port_id().id(); }
    const char* name() { return data_.name().c_str(); }
    void protoDataCopyInto(OstProto::Port *port) { port->CopyFrom(data_); }

    bool modify(const OstProto::Port &port);

    virtual OstProto::LinkState linkState() { return linkState_; }
    virtual bool hasExclusiveControl() = 0;
    virtual bool setExclusiveControl(bool exclusive) = 0;

    int streamCount() { return streamList_.size(); }
    StreamBase* streamAtIndex(int index);
    StreamBase* stream(int streamId);
    bool addStream(StreamBase *stream);
    bool deleteStream(int streamId);

    bool isDirty() { return isSendQueueDirty_; }
    void setDirty() { isSendQueueDirty_ = true; }

    Accuracy rateAccuracy();
    virtual bool setRateAccuracy(Accuracy accuracy);

    virtual void clearPacketList() = 0;
    virtual void loopNextPacketSet(qint64 size, qint64 repeats,
            long repeatDelaySec, long repeatDelayNsec) = 0;
    virtual bool appendToPacketList(long sec, long nsec, const uchar *packet, 
            int length) = 0;
    virtual void setPacketListLoopMode(bool loop, 
            quint64 secDelay, quint64 nsecDelay) = 0;
    void updatePacketList();

    virtual void startTransmit() = 0;
    virtual void stopTransmit() = 0;
    virtual bool isTransmitOn() = 0;

    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCaptureOn() = 0;
    virtual QIODevice* captureData() = 0;

    void stats(PortStats *stats);
    void resetStats() { epochStats_ = stats_; }

    DeviceManager* deviceManager();
    virtual void startDeviceEmulation() = 0;
    virtual void stopDeviceEmulation() = 0;
    virtual int sendEmulationPacket(PacketBuffer *pktBuf) = 0;

    void clearDeviceNeighbors();
    void resolveDeviceNeighbors();

    quint64 deviceMacAddress(int streamId, int frameIndex);
    quint64 neighborMacAddress(int streamId, int frameIndex);

protected:
    void addNote(QString note);

    void updatePacketListSequential();
    void updatePacketListInterleaved();

    bool isUsable_;
    OstProto::Port          data_;
    OstProto::LinkState     linkState_;
    ulong minPacketSetSize_;
    Accuracy rateAccuracy_;

    quint64 maxStatsValue_;
    struct PortStats    stats_;
    //! \todo Need lock for stats access/update

    DeviceManager *deviceManager_;

private:
    bool    isSendQueueDirty_;

    static const int kMaxPktSize = 16384;
    uchar   pktBuf_[kMaxPktSize];

    // When finding a corresponding device for a packet, we need to inspect
    // only uptil the L3 header; in the worst case this would be -
    // mac (12) + 4 x vlan (16) + ethType (2) + ipv6 (40) = 74 bytes
    // let's round it up to 80 bytes
    static const int kMaxL3PktSize = 80;

    /*! \note StreamBase::id() and index into streamList[] are NOT same! */
    QList<StreamBase*>  streamList_;

    struct PortStats    epochStats_;

};

#endif
