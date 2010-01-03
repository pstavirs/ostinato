#include "streambase.h"
#include "abstractprotocol.h"
#include "protocollist.h"
#include "protocollistiterator.h"
#include "protocolmanager.h"

extern ProtocolManager OstProtocolManager;

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
    proto = OstProtocolManager.createProtocol(
            OstProto::Protocol::kMacFieldNumber, this);
    iter->insert(proto);
    qDebug("stream: mac = %p", proto);

    proto = OstProtocolManager.createProtocol(
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
    delete mStreamId;
    delete mCore;
    delete mControl;
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
        proto = OstProtocolManager.createProtocol(
            stream.protocol(i).protocol_id().id(), this);
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

bool StreamBase::operator < (const StreamBase &s) const
{
    return(mCore->ordinal() < s.mCore->ordinal());
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
            qsrand(((uint) this));
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

quint32 StreamBase::packetRate() const
{
    return (quint32) mControl->packets_per_sec();
}

bool StreamBase::setPacketRate(quint32 packetsPerSec)
{
    mControl->set_packets_per_sec(packetsPerSec); 
    return true;
}

quint32 StreamBase::burstRate() const
{
    return (quint32) mControl->bursts_per_sec();
}

bool StreamBase::setBurstRate(quint32 burstsPerSec)
{
    mControl->set_bursts_per_sec(burstsPerSec); 
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

int StreamBase::frameValue(uchar *buf, int bufMaxSize, int n) const
{
    int        pktLen, len = 0;

    pktLen = frameLen(n);

    // pktLen is adjusted for CRC/FCS which will be added by the NIC
    pktLen -=  4;

    if ((pktLen < 0) || (pktLen > bufMaxSize))
        return 0;

    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol    *proto;
        QByteArray            ba;

        proto = iter->next();
        ba = proto->protocolFrameValue(n);

        if (len + ba.size() < bufMaxSize)
            memcpy(buf+len, ba.constData(), ba.size());
        len += ba.size();
    }
    delete iter;

    return pktLen;
}

