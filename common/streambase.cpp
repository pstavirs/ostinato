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

#include "streambase.h"
#include "abstractprotocol.h"
#include "protocollist.h"
#include "protocollistiterator.h"
#include "protocolmanager.h"

extern ProtocolManager *OstProtocolManager;

StreamBase::StreamBase() :
    mStreamId(new OstProto::StreamId),
    mCore(new OstProto::StreamCore),
    mControl(new OstProto::StreamControl)
{
    AbstractProtocol *proto;
    ProtocolListIterator *iter;

    mStreamId->set_id(0xFFFFFFFF);

    currentFrameProtocols = new ProtocolList;

    iter = createProtocolListIterator();
    // By default newly created streams have the mac and payload protocols
    proto = OstProtocolManager->createProtocol(
            OstProto::Protocol::kMacFieldNumber, this);
    iter->insert(proto);
    qDebug("stream: mac = %p", proto);

    proto = OstProtocolManager->createProtocol(
            OstProto::Protocol::kPayloadFieldNumber, this);
    iter->insert(proto);
    qDebug("stream: payload = %p", proto);

    {
        iter->toFront();
        while (iter->hasNext())
        {
            qDebug("{{%p}}", iter->next());
        //    qDebug("{{%p}: %d}", iter->peekNext(), iter->next()->protocolNumber());
        }
        iter->toFront();
        while (iter->hasNext())
        {
            qDebug("{[%d]}", iter->next()->protocolNumber());
        //    qDebug("{{%p}: %d}", iter->peekNext(), iter->next()->protocolNumber());
        }
    }

    delete iter;
}

StreamBase::~StreamBase()
{
    currentFrameProtocols->destroy();
    delete currentFrameProtocols;
    delete mControl;
    delete mCore;
    delete mStreamId;
}

void StreamBase::protoDataCopyFrom(const OstProto::Stream &stream)
{
    AbstractProtocol        *proto;
    ProtocolListIterator    *iter;

    mStreamId->CopyFrom(stream.stream_id());
    mCore->CopyFrom(stream.core());
    mControl->CopyFrom(stream.control());

    currentFrameProtocols->destroy();
    iter = createProtocolListIterator();
    for (int i=0; i < stream.protocol_size(); i++)
    {
        int protoId = stream.protocol(i).protocol_id().id();

        if (!OstProtocolManager->isRegisteredProtocol(protoId))
        {
            qWarning("Skipping unregistered protocol %d", protoId);
            continue;
        }
        proto = OstProtocolManager->createProtocol(protoId, this);
        proto->protoDataCopyFrom(stream.protocol(i));
        iter->insert(proto);
    }

    delete iter;
}

void StreamBase::protoDataCopyInto(OstProto::Stream &stream) const
{
    stream.mutable_stream_id()->CopyFrom(*mStreamId);
    stream.mutable_core()->CopyFrom(*mCore);
    stream.mutable_control()->CopyFrom(*mControl);

    stream.clear_protocol();
    foreach (const AbstractProtocol* proto, *currentFrameProtocols)
    {
        OstProto::Protocol *p;

        p = stream.add_protocol();
        proto->protoDataCopyInto(*p);
    }
}

#if 0
ProtocolList StreamBase::frameProtocol()
{
    return currentFrameProtocols;
}

void StreamBase::setFrameProtocol(ProtocolList protocolList)
{
    //currentFrameProtocols.destroy();
    currentFrameProtocols = protocolList;
}
#endif

ProtocolListIterator*  StreamBase::createProtocolListIterator() const
{
    return new ProtocolListIterator(*currentFrameProtocols);
}

quint32    StreamBase::id()
{
    return mStreamId->id();
}

bool StreamBase::setId(quint32 id)
{
    mStreamId->set_id(id);
    return true;
}

quint32    StreamBase::ordinal()
{
    return mCore->ordinal();
}

bool StreamBase::setOrdinal(quint32    ordinal)
{
    mCore->set_ordinal(ordinal);
    return true;
}

bool StreamBase::isEnabled() const
{
    return mCore->is_enabled();
}

bool StreamBase::setEnabled(bool flag)
{
    mCore->set_is_enabled(flag);
    return true;
}

const QString StreamBase::name() const 
{
    return QString().fromStdString(mCore->name());
}

bool StreamBase::setName(QString name) 
{
    mCore->set_name(name.toStdString());
    return true;
}

StreamBase::FrameLengthMode    StreamBase::lenMode() const
{
    return (StreamBase::FrameLengthMode) mCore->len_mode();
}

bool StreamBase::setLenMode(FrameLengthMode    lenMode)
{
    mCore->set_len_mode((OstProto::StreamCore::FrameLengthMode) lenMode); 
    return true;
}

quint16    StreamBase::frameLen(int streamIndex) const
{
    int        pktLen;

    // Decide a frame length based on length mode
    switch(lenMode())
    {
        case OstProto::StreamCore::e_fl_fixed:
            pktLen = mCore->frame_len();
            break;
        case OstProto::StreamCore::e_fl_inc:
            pktLen = frameLenMin() + (streamIndex %
                (frameLenMax() - frameLenMin() + 1));
            break;
        case OstProto::StreamCore::e_fl_dec:
            pktLen = frameLenMax() - (streamIndex %
                (frameLenMax() - frameLenMin() + 1));
            break;
        case OstProto::StreamCore::e_fl_random:
            //! \todo (MED) This 'random' sequence is same across iterations
            pktLen = 64; // to avoid the 'maybe used uninitialized' warning
            qsrand(reinterpret_cast<ulong>(this));
            for (int i = 0; i <= streamIndex; i++)
                pktLen = qrand();
            pktLen = frameLenMin() + (pktLen %
                (frameLenMax() - frameLenMin() + 1));
            break;
        default:
            qWarning("Unhandled len mode %d. Using default 64", 
                    lenMode());
            pktLen = 64;
            break;
    }

    return pktLen;
}

bool StreamBase::setFrameLen(quint16 frameLen)
{
    mCore->set_frame_len(frameLen);  
    return true;
}

quint16    StreamBase::frameLenMin() const
{
    return mCore->frame_len_min();
}

bool StreamBase::setFrameLenMin(quint16 frameLenMin)
{
    mCore->set_frame_len_min(frameLenMin);  
    return true;
}

quint16    StreamBase::frameLenMax() const
{
    return mCore->frame_len_max();
}

bool StreamBase::setFrameLenMax(quint16 frameLenMax)
{
    mCore->set_frame_len_max(frameLenMax);  
    return true;
}

/*! Convenience Function */
quint16 StreamBase::frameLenAvg() const
{
    quint16 avgFrameLen;

    if (lenMode() == e_fl_fixed)
        avgFrameLen = frameLen();
    else
        avgFrameLen = (frameLenMin() + frameLenMax())/2;

    return avgFrameLen;
}

StreamBase::SendUnit StreamBase::sendUnit() const
{
    return (StreamBase::SendUnit) mControl->unit();
}

bool StreamBase::setSendUnit(SendUnit sendUnit)
{
    mControl->set_unit((OstProto::StreamControl::SendUnit) sendUnit); 
    return true;
}

StreamBase::SendMode StreamBase::sendMode() const
{
    return (StreamBase::SendMode) mControl->mode();
}

bool StreamBase::setSendMode(SendMode sendMode)
{
    mControl->set_mode(
        (OstProto::StreamControl::SendMode) sendMode); 
    return true;
}

StreamBase::NextWhat StreamBase::nextWhat() const
{
    return (StreamBase::NextWhat) mControl->next();
}

bool StreamBase::setNextWhat(NextWhat nextWhat)
{
    mControl->set_next((OstProto::StreamControl::NextWhat) nextWhat); 
    return true;
}

quint32 StreamBase::numPackets() const
{
    return (quint32) mControl->num_packets();
}

bool StreamBase::setNumPackets(quint32 numPackets)
{
    mControl->set_num_packets(numPackets); 
    return true;
}

quint32 StreamBase::numBursts() const
{
    return (quint32) mControl->num_bursts();
}

bool StreamBase::setNumBursts(quint32 numBursts)
{
    mControl->set_num_bursts(numBursts); 
    return true;
}

quint32 StreamBase::burstSize() const
{
    return (quint32) mControl->packets_per_burst();
}

bool StreamBase::setBurstSize(quint32 packetsPerBurst)
{
    mControl->set_packets_per_burst(packetsPerBurst); 
    return true;
}

double StreamBase::packetRate() const
{
    return (double) mControl->packets_per_sec();
}

bool StreamBase::setPacketRate(double packetsPerSec)
{
    mControl->set_packets_per_sec(packetsPerSec); 
    return true;
}

double StreamBase::burstRate() const
{
    return (double) mControl->bursts_per_sec();
}

bool StreamBase::setBurstRate(double burstsPerSec)
{
    mControl->set_bursts_per_sec(burstsPerSec); 
    return true;
}

/*! Convenience Function */
double StreamBase::averagePacketRate() const
{
    double avgPacketRate;

    switch (sendUnit())
    {
    case e_su_bursts:
        avgPacketRate = burstRate() * burstSize();
        break;
    case e_su_packets:
        avgPacketRate = packetRate();
        break;
    default:
        Q_ASSERT(false); // Unreachable!!
    }

    return avgPacketRate;
}

/*! Convenience Function */
bool StreamBase::setAveragePacketRate(double packetsPerSec)
{
    switch (sendUnit())
    {
    case e_su_bursts:
        setBurstRate(packetsPerSec/double(burstSize()));
        break;
    case e_su_packets:
        setPacketRate(packetsPerSec);
        break;
    default:
        Q_ASSERT(false); // Unreachable!!
    }

    return true;
}

bool StreamBase::isFrameVariable() const
{
    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;

        proto = iter->next();
        if (proto->isProtocolFrameValueVariable())
            goto _exit;
    }
    delete iter;
    return false;

_exit:
    delete iter;
    return true;
}

bool StreamBase::isFrameSizeVariable() const
{
    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;

        proto = iter->next();
        if (proto->isProtocolFrameSizeVariable())
            goto _exit;
    }
    delete iter;
    return false;

_exit:
    delete iter;
    return true;
}

// frameProtocolLength() returns the sum of all the individual protocol sizes
// which may be different from frameLen()
int StreamBase::frameProtocolLength(int frameIndex) const
{
    int len = 0;
    ProtocolListIterator *iter = createProtocolListIterator();

    while (iter->hasNext())
    {
        AbstractProtocol *proto = iter->next();

        len += proto->protocolFrameSize(frameIndex);
    }
    delete iter;

    return len;
}

int StreamBase::frameCount() const
{
    int count = 0;

    switch (sendUnit())
    {
    case e_su_packets: count = numPackets(); break;
    case e_su_bursts: count =  numBursts() * burstSize(); break;
    default: Q_ASSERT(false); // unreachable
    }

    return count;
}

int StreamBase::frameValue(uchar *buf, int bufMaxSize, int frameIndex) const
{
    int        pktLen, len = 0;

    pktLen = frameLen(frameIndex);

    // pktLen is adjusted for CRC/FCS which will be added by the NIC
    pktLen -= kFcsSize;

    if ((pktLen < 0) || (pktLen > bufMaxSize))
        return 0;

    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;
        QByteArray            ba;

        proto = iter->next();
        ba = proto->protocolFrameValue(frameIndex);

        if (len + ba.size() < bufMaxSize)
            memcpy(buf+len, ba.constData(), ba.size());
        len += ba.size();
    }
    delete iter;

    // Pad with zero, if required
    if (len < pktLen)
        memset(buf+len, 0, pktLen-len);

    return pktLen;
}

bool StreamBase::preflightCheck(QString &result) const
{
    bool pass = true;
    int count = isFrameSizeVariable() ? frameCount() : 1;

    for (int i = 0; i < count; i++)
    {
        if (frameLen(i) < (frameProtocolLength(i) + kFcsSize))
        {
            result += QString("One or more frames may be truncated - "
                "frame length should be at least %1.\n")
                .arg(frameProtocolLength(i) + kFcsSize);
            pass = false;
        }

        if (frameLen(i) > 1522)
        {
            result += QString("Jumbo frames may be truncated or dropped "
                "if not supported by the hardware\n");
            pass = false;
        }
    }

    return pass;
}

bool StreamBase::StreamLessThan(StreamBase* stream1, StreamBase* stream2)
{
    return stream1->ordinal() < stream2->ordinal() ? true : false;
}
