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
#include "framevalueattrib.h"
#include "protocollist.h"
#include "protocollistiterator.h"
#include "protocolmanager.h"
#include "uint128.h"

#include <QDebug>

extern ProtocolManager *OstProtocolManager;
extern quint64 getDeviceMacAddress(int portId, int streamId, int frameIndex);
extern quint64 getNeighborMacAddress(int portId, int streamId, int frameIndex);

StreamBase::StreamBase(int portId) :
    portId_(portId),
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

#ifndef QT_NO_DEBUG_OUTPUT
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
#endif

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
        proto->commonProtoDataCopyFrom(stream.protocol(i));
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
        proto->commonProtoDataCopyInto(*p);
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

bool StreamBase::hasProtocol(quint32 protocolNumber) const
{
    foreach(const AbstractProtocol *proto, *currentFrameProtocols)
        if (proto->protocolNumber() == protocolNumber)
            return true;

    return false;
}

ProtocolListIterator*  StreamBase::createProtocolListIterator() const
{
    return new ProtocolListIterator(*currentFrameProtocols);
}

quint32 StreamBase::id() const
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
        case e_fl_fixed:
            pktLen = mCore->frame_len();
            break;
        case e_fl_inc:
            pktLen = frameLenMin() + (streamIndex %
                (frameLenMax() - frameLenMin() + 1));
            break;
        case e_fl_dec:
            pktLen = frameLenMax() - (streamIndex %
                (frameLenMax() - frameLenMin() + 1));
            break;
        case e_fl_random:
            //! \todo (MED) This 'random' sequence is same across iterations
            pktLen = 64; // to avoid the 'maybe used uninitialized' warning
            qsrand(reinterpret_cast<ulong>(this));
            for (int i = 0; i <= streamIndex; i++)
                pktLen = qrand();
            pktLen = frameLenMin() + (pktLen %
                (frameLenMax() - frameLenMin() + 1));
            break;
        case e_fl_imix: {
            // 64, 594, 1518 in 7:4:1 ratio
            // sizes mixed up intentionally below
            static int imixPattern[12]
                = {64, 594, 64, 594, 64, 1518, 64, 64, 594, 64, 594, 64};
            pktLen = imixPattern[streamIndex % 12];
            break;
        }
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
    else if (lenMode() == e_fl_imix)
        avgFrameLen = (7*64 + 4*594 + 1*1518)/12; // 64,594,1518 in 7:4:1 ratio
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
    double avgPacketRate = 0;

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

int StreamBase::frameSizeVariableCount() const
{
    int count = 1;

    switch(lenMode())
    {
        case e_fl_fixed:
            break;
        case e_fl_inc:
        case e_fl_dec:
        case e_fl_random:
            count = qMin(frameLenMax() - frameLenMin() + 1, frameCount());
            break;
        case e_fl_imix:
            count = 12; // 7:4:1 ratio, so 7+4+1
            break;
        default:
            qWarning("%s: Unhandled len mode %d",  __FUNCTION__, lenMode());
            break;
    }

    return count;
}

int StreamBase::frameVariableCount() const
{
    ProtocolListIterator    *iter;
    quint64 frameCount = 1;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;
        int count;

        proto = iter->next();
        count = proto->protocolFrameVariableCount();

        // correct count for mis-behaving protocols
        if (count <= 0)
            count = 1;

        frameCount = AbstractProtocol::lcm(frameCount, count);
    }
    delete iter;

    return AbstractProtocol::lcm(frameCount, frameSizeVariableCount());
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

// Returns packet length - if bufMaxSize < frameLen(), returns truncated
// length i.e. bufMaxSize
int StreamBase::frameValue(uchar *buf, int bufMaxSize, int frameIndex,
        FrameValueAttrib *attrib) const
{
    int maxSize, size, pktLen, len = 0;

    pktLen = frameLen(frameIndex);

    // pktLen is adjusted for CRC/FCS which will be added by the NIC
    pktLen -= kFcsSize;

    if (pktLen <= 0)
        return 0;

    maxSize = qMin(pktLen, bufMaxSize);

    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;
        QByteArray          ba;
        FrameValueAttrib    protoAttrib;

        proto = iter->next();
        ba = proto->protocolFrameValue(frameIndex, false, &protoAttrib);
        if (attrib)
            *attrib += protoAttrib;

        size = qMin(ba.size(), maxSize-len);
        memcpy(buf+len, ba.constData(), size);
        len += size;

        if (len == maxSize)
            break;
    }
    delete iter;

    // Pad with zero, if required and if we have space
    if (len < maxSize) {
        size = maxSize-len;
        memset(buf+len, 0, size);
        len += size;
    }

    return len;
}

template <typename T>
int StreamBase::findReplace(quint32 protocolNumber, int fieldIndex,
        QVariant findValue, QVariant findMask,
        QVariant replaceValue, QVariant replaceMask)
{
    int replaceCount = 0;
    ProtocolListIterator *iter = createProtocolListIterator();

    // FIXME: Because protocol list iterator is unaware of combo protocols
    // search for ip4.src will NOT succeed in a combo protocol containing ip4
    while (iter->hasNext()) {
        AbstractProtocol *proto = iter->next();
        if (proto->protocolNumber() != protocolNumber)
            continue;

        T fieldValue = proto->fieldData(fieldIndex,
                                    AbstractProtocol::FieldValue).value<T>();
        qDebug() << "findReplace:"
                 << "stream" <<  mStreamId->id()
                 << "field" << fieldValue
                 << "findMask" << hex << findMask.value<T>() << dec
                 << "findValue" << findValue.value<T>();
        if ((fieldValue & findMask.value<T>()) == findValue.value<T>()) {
            T newValue = (fieldValue & ~replaceMask.value<T>())
                        | (replaceValue.value<T>() & replaceMask.value<T>());
            qDebug() << "findReplace:"
                     << "replaceMask" << hex << replaceMask.value<T>() << dec
                     << "replaceValue" << replaceValue.value<T>()
                     << "newValue" << newValue;

            QVariant nv;
            nv.setValue(newValue);
            if (proto->setFieldData(fieldIndex, nv))
                replaceCount++;
        }
    }
    delete iter;

    return replaceCount;
}

int StreamBase::protocolFieldReplace(quint32 protocolNumber,
        int fieldIndex, int fieldBitSize,
        QVariant findValue, QVariant findMask,
        QVariant replaceValue, QVariant replaceMask)
{
    if (fieldBitSize <= 64)
        return findReplace<qulonglong>(protocolNumber, fieldIndex,
                findValue, findMask, replaceValue, replaceMask);

    if (fieldBitSize == 128)
        return findReplace<UInt128>(protocolNumber, fieldIndex,
                findValue, findMask, replaceValue, replaceMask);

    qWarning("Unknown find/replace type %d", findValue.type());
    return 0;
}

quint64 StreamBase::deviceMacAddress(int frameIndex) const
{
    return getDeviceMacAddress(portId_, int(mStreamId->id()), frameIndex);
}

quint64 StreamBase::neighborMacAddress(int frameIndex) const
{
    return getNeighborMacAddress(portId_, int(mStreamId->id()), frameIndex);
}

/*!
  Checks for any potential errors with the packets generated by this
  stream. Returns true if no problems are found, false otherwise. Details
  of the error(s) are available in the INOUT param result

  All errors found are returned. However, each type of error is reported
  only once, even if multiple packets may have that error.
*/
bool StreamBase::preflightCheck(QStringList &result) const
{
    bool pass = true;
    bool chkShort = true;
    bool chkTrunc = true;
    bool chkJumbo = true;
    int count = isFrameSizeVariable() ? frameSizeVariableCount() : 1;

    for (int i = 0; i < count; i++)
    {
        int pktLen = frameLen(i);

        if (chkShort && hasProtocol(OstProto::Protocol::kSignFieldNumber)
                && (pktLen > (frameProtocolLength(i) + kFcsSize)))
        {
            result << QObject::tr("Stream statistics may not work since "
                    "frame content &lt; 64 bytes and hence will get padded - "
                    "make sure special signature is at the end of the "
                    "frame and frame content &ge; 64 bytes");
            chkShort = false;
            pass = false;
        }

        if (chkTrunc && (pktLen < (frameProtocolLength(i) + kFcsSize)))
        {
            result << QObject::tr("One or more frames may be truncated - "
                "frame length should be at least %1")
                .arg(frameProtocolLength(i) + kFcsSize);
            chkTrunc = false;
            pass = false;
        }

        if (chkJumbo && (pktLen > 1522))
        {
            result << QObject::tr("Jumbo frames may be truncated or dropped "
                "if not supported by the hardware");
            chkJumbo = false;
            pass = false;
        }

        // Break out of loop if we've seen at least one instance of all
        // the above errors
        if (!chkTrunc && !chkJumbo)
            break;
    }

    ProtocolListIterator *iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        QStringList errors;
        AbstractProtocol *proto = iter->next();
        if (proto->hasErrors(&errors)) {
            result += errors;
            pass = false;
        }
    }
    delete iter;

    if (isFrameVariable()) {
        if (frameVariableCount() > frameCount())
        {
            if (frameCount() == 1)
            {
                result << QObject::tr("Variable fields won't change since "
                        "only 1 frame%1 is configured to be transmitted - "
                        "increase number of packets to %L2 to have variable "
                        "fields change across the configured range")
                    .arg(sendUnit() == e_su_bursts ?
                            " (number of bursts * packets per burst)" :
                            "")
                    .arg(frameVariableCount());
                pass = false;
            }
            else if (frameCount() > 1)
            {
                result << QObject::tr("Variable fields will change for "
                        "%L1 counts since only %L1 frames%2 are configured "
                        "to be transmitted - increase number of packets "
                        "from %L1 to %L3 to have variable fields change "
                        "across the configured range")
                    .arg(frameCount())
                    .arg(sendUnit() == e_su_bursts ?
                            " (number of bursts * packets per burst)" :
                            "")
                    .arg(frameVariableCount());
                pass = false;
            }
        }
    }

#if 0 // see XXX note below
    // XXX: This causes false positives for -
    // * interleaved streams (a port property that we don't have access to)
    // * pcap imported streams where each stream has only one packet
    // Ideally we need to get the transmit duration for all the streams
    // to perform this check
    if (frameCount() < averagePacketRate() && nextWhat() != e_nw_goto_id)
    {
        result << QObject::tr("Only %L1 frames at the rate of "
                "%L2 frames/sec are configured to be transmitted. "
                "Transmission will last for only %L3 second - "
                "to transmit for a longer duration, "
                "increase the number of packets (bursts) and/or "
                "set the 'After this stream' action as 'Goto First'")
            .arg(frameCount())
            .arg(averagePacketRate(), 0, 'f', 2)
            .arg(frameCount()/averagePacketRate(), 0, 'f');
        pass = false;
    }
#endif

    return pass;
}

bool StreamBase::StreamLessThan(StreamBase* stream1, StreamBase* stream2)
{
    return stream1->ordinal() < stream2->ordinal() ? true : false;
}
