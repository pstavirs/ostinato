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

#include "pdml_p.h"

#include "abstractprotocol.h"
#include "pcapfileformat.h"
#include "protocolmanager.h"
#include "streambase.h"

#include "arp.pb.h"
#include "eth2.pb.h"
#include "dot3.pb.h"
#include "hexdump.pb.h"
#include "llc.pb.h"
#include "mac.pb.h"
#include "ip4.pb.h"
#include "ip6.pb.h"
#include "snap.pb.h"
#include "svlan.pb.h"
#include "tcp.pb.h"
#include "textproto.pb.h"
#include "udp.pb.h"
#include "vlan.pb.h"

#include <google/protobuf/descriptor.h>

#include <QMessageBox>
#include <QRegExp>

#include <string>

extern ProtocolManager *OstProtocolManager;

const int kBaseHex = 16;

static PdmlReader *gPdmlReader = NULL;

PdmlDefaultProtocol::PdmlDefaultProtocol()
{
    ostProtoId_ = -1;
}

PdmlDefaultProtocol::~PdmlDefaultProtocol()
{
}

PdmlDefaultProtocol* PdmlDefaultProtocol::createInstance()
{
    return new PdmlDefaultProtocol();
}

QString PdmlDefaultProtocol::pdmlProtoName() const
{
    return pdmlProtoName_;
}

int PdmlDefaultProtocol::ostProtoId() const
{
    return ostProtoId_;
}

bool PdmlDefaultProtocol::hasField(QString name) const
{
    return fieldMap_.contains(name);
}

int PdmlDefaultProtocol::fieldId(QString name) const
{
    return fieldMap_.value(name);
}

void PdmlDefaultProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, 
        int /*expectedPos*/, OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::prematureEndHandler(int /*pos*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::postProtocolHandler(OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::fieldHandler(QString name, 
        const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (hasField(name))
    {
        QString valueHexStr = attributes.value("value").toString();

        qDebug("\t(KNOWN) fieldName:%s, value:%s",
                name.toAscii().constData(), 
                valueHexStr.toAscii().constData());

        knownFieldHandler(name, valueHexStr, pbProto);
    }
    else
    {
        int pos = -1;
        int size = -1;

        if (!attributes.value("pos").isEmpty())
            pos = attributes.value("pos").toString().toInt();
        if (!attributes.value("size").isEmpty())
            size = attributes.value("size").toString().toInt();

        qDebug("\t(UNKNOWN) fieldName:%s, pos:%d, size:%d",
                name.toAscii().constData(), pos, size);

        unknownFieldHandler(name, pos, size, attributes, pbProto, stream);
    }
}

void PdmlDefaultProtocol::knownFieldHandler(QString name, QString valueHexStr,
        OstProto::Protocol *pbProto)
{
    const google::protobuf::Reflection *protoRefl = pbProto->GetReflection();
    const google::protobuf::FieldDescriptor *extDesc = 
                protoRefl->FindKnownExtensionByNumber(ostProtoId());

    google::protobuf::Message *msg = 
                protoRefl->MutableMessage(pbProto,extDesc);

    const google::protobuf::Reflection *msgRefl = msg->GetReflection();
    const google::protobuf::FieldDescriptor *fieldDesc = 
                msg->GetDescriptor()->FindFieldByNumber(fieldId(name));

    bool isOk;

    Q_ASSERT(fieldDesc != NULL);
    switch(fieldDesc->cpp_type())
    {
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // TODO
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        msgRefl->SetUInt32(msg, fieldDesc, 
                valueHexStr.toUInt(&isOk, kBaseHex));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        msgRefl->SetUInt64(msg, fieldDesc, 
                valueHexStr.toULongLong(&isOk, kBaseHex));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    {
        QByteArray hexVal = QByteArray::fromHex(valueHexStr.toUtf8());
        std::string str(hexVal.constData(), hexVal.size());
        msgRefl->SetString(msg, fieldDesc, str);
        break;
    }
    default:
        qDebug("%s: unhandled cpptype = %d", __FUNCTION__, 
                fieldDesc->cpp_type());
    }
}

void PdmlDefaultProtocol::unknownFieldHandler(QString name, 
        int pos, int size, const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    return; // do nothing!
}


// ---------------------------------------------------------- //
// PdmlReader                                                 //
// ---------------------------------------------------------- //
PdmlReader::PdmlReader(OstProto::StreamConfigList *streams)
{
    gPdmlReader = this;
    pcap_ = NULL;
    streams_ = streams;

    currentStream_ = NULL;
    prevStream_ = NULL;

    stop_ = NULL;

    factory_.insert("hexdump", PdmlUnknownProtocol::createInstance);
    factory_.insert("geninfo", PdmlGenInfoProtocol::createInstance);
    factory_.insert("frame", PdmlFrameProtocol::createInstance);

    factory_.insert("arp", PdmlArpProtocol::createInstance);
    factory_.insert("eth", PdmlEthProtocol::createInstance);
    factory_.insert("http", PdmlTextProtocol::createInstance);
    factory_.insert("ieee8021ad", PdmlSvlanProtocol::createInstance);
    factory_.insert("ip", PdmlIp4Protocol::createInstance);
    factory_.insert("ipv6", PdmlIp6Protocol::createInstance);
    factory_.insert("llc", PdmlLlcProtocol::createInstance);
    factory_.insert("nntp", PdmlTextProtocol::createInstance);
    factory_.insert("rtsp", PdmlTextProtocol::createInstance);
    factory_.insert("sip", PdmlTextProtocol::createInstance);
    factory_.insert("tcp", PdmlTcpProtocol::createInstance);
    factory_.insert("udp", PdmlUdpProtocol::createInstance);
    factory_.insert("udplite", PdmlUdpProtocol::createInstance);
    factory_.insert("vlan", PdmlVlanProtocol::createInstance);
}

PdmlReader::~PdmlReader()
{
}

bool PdmlReader::read(QIODevice *device, PcapFileFormat *pcap, bool *stop)
{
    setDevice(device);
    pcap_ = pcap;
    stop_ = stop;

    while (!atEnd())
    {
        readNext();
        if (isStartElement())
        {
            if (name() == "pdml")
                readPdml();
            else
                raiseError("Not a pdml file!");
        }
    }

    if (error() && (errorString() != "USER-CANCEL"))
    {
        qDebug("Line %lld", lineNumber());
        qDebug("Col %lld", columnNumber());
        qDebug("%s", errorString().toAscii().constData());
        return false;
    }
    return true;
}

// TODO: use a temp pool to avoid a lot of new/delete
PdmlDefaultProtocol* PdmlReader::allocPdmlProtocol(QString protoName)
{
    // If protoName is not known, we use a hexdump
    if (!factory_.contains(protoName))
        protoName = "hexdump";

    return (*(factory_.value(protoName)))();
}

void PdmlReader::freePdmlProtocol(PdmlDefaultProtocol *proto)
{
    delete proto;
}

bool PdmlReader::isDontCareProto()
{
    Q_ASSERT(isStartElement() && name() == "proto");

    QStringRef protoName = attributes().value("name");

    if (protoName.isEmpty() || (protoName == "expert"))
        return true;

    return false;
}

void PdmlReader::skipElement()
{
    Q_ASSERT(isStartElement());

    qDebug("skipping element - <%s>", 
                    name().toString().toAscii().constData());
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            skipElement();
    }
}

void PdmlReader::readPdml()
{
    Q_ASSERT(isStartElement() && name() == "pdml");

    packetCount_ = 1;

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "packet")
                readPacket();
            else
                skipElement();
        }
    }
}

void PdmlReader::readPacket()
{
    PcapFileFormat::PcapPacketHeader pktHdr;

    Q_ASSERT(isStartElement() && name() == "packet");

    qDebug("%s: packetNum = %d", __FUNCTION__, packetCount_);

    skipUntilEnd_ = false;

    // XXX: we play dumb and convert each packet to a stream, for now
    prevStream_ = currentStream_;
    currentStream_ = streams_->add_stream();
    currentStream_->mutable_stream_id()->set_id(packetCount_);
    currentStream_->mutable_core()->set_is_enabled(true);

    // Set to a high number; will get reset to correct value during parse
    currentStream_->mutable_core()->set_frame_len(16384); // FIXME: Hard coding!

    expPos_ = 0;

    if (pcap_)
        pcap_->readPacket(pktHdr, pktBuf_);

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (skipUntilEnd_)
                skipElement();
            else if (name() == "proto")
                readProto();
            else if (name() == "field")
                readField(NULL, NULL); // TODO: top level field!!!!
            else 
                skipElement();
        }
    }

    currentStream_->mutable_core()->set_name(""); // FIXME

    // If trailing bytes are missing, add those from the pcap 
    if ((expPos_ < pktBuf_.size()) && pcap_)
    {
        OstProto::Protocol *proto = currentStream_->add_protocol();
        OstProto::HexDump *hexDump = proto->MutableExtension(
                OstProto::hexDump);

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        qDebug("adding trailing %d bytes starting from %d",
               pktBuf_.size() - expPos_, expPos_); 
        hexDump->set_content(pktBuf_.constData() + expPos_, 
                pktBuf_.size() - expPos_);
        hexDump->set_pad_until_end(false);
    } 

    packetCount_++;
    emit progress(int(characterOffset()*100/device()->size())); // in % 
    if (prevStream_)
        prevStream_->mutable_control()->CopyFrom(currentStream_->control());
    if (stop_ && *stop_)
        raiseError("USER-CANCEL");
}

void PdmlReader::readProto()
{
    PdmlDefaultProtocol *pdmlProto = NULL;
    OstProto::Protocol *pbProto = NULL;

    Q_ASSERT(isStartElement() && name() == "proto");

    QString protoName;
    int pos = -1;
    int size = -1;

    if (!attributes().value("name").isEmpty())
        protoName = attributes().value("name").toString();
    if (!attributes().value("pos").isEmpty())
        pos = attributes().value("pos").toString().toInt();
    if (!attributes().value("size").isEmpty())
        size = attributes().value("size").toString().toInt();

    qDebug("proto: %s, pos = %d, expPos_ = %d, size = %d", 
            protoName.toAscii().constData(), pos, expPos_, size);

    // This is a heuristic to skip protocols which are not part of
    // this frame, but of a reassembled segment spanning several frames
    //   1. Proto starting pos is 0, but we've already seen some protocols
    //   2. Protocol Size exceeds frame length
    if (((pos == 0) && (currentStream_->protocol_size() > 0))
        || ((pos + size) > int(currentStream_->core().frame_len())))
    {
        skipElement();
        return;
    }

    if (isDontCareProto())
    {
        skipElement();
        return;
    }

    // if we detect a gap between subsequent protocols, we "fill-in"
    // with a "hexdump" from the pcap
    if (pos > expPos_ && pcap_)
    {
        appendHexDumpProto(expPos_, pos - expPos_);
        expPos_ = pos;
    }

    // for unknown protocol, read a hexdump from the pcap
    if (!factory_.contains(protoName) && pcap_)
    {
        int size = -1;

        if (!attributes().value("size").isEmpty())
            size = attributes().value("size").toString().toInt();

        // Check if this proto is a subset of previous proto - if so, do nothing
        if ((pos >= 0) && (size > 0) && ((pos + size) <= expPos_))
        {
            qDebug("subset proto");
            skipElement();
            return;
        }

        if (pos >= 0 && size > 0 
                && ((pos + size) <= pktBuf_.size()))
        {
            appendHexDumpProto(pos, size);
            expPos_ += size;

            skipElement();
            return;
        }
    }

    pdmlProto = appendPdmlProto(protoName, &pbProto);

    qDebug("%s: preProtocolHandler(expPos = %d)", 
            protoName.toAscii().constData(), expPos_);
    pdmlProto->preProtocolHandler(protoName, attributes(), expPos_, pbProto,
            currentStream_);

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "proto")
            {
                // an embedded proto
                qDebug("embedded proto: %s\n", attributes().value("name")
                        .toString().toAscii().constData());

                if (isDontCareProto())
                {
                    skipElement();
                    continue;
                }

                // if we are in the midst of processing a protocol, we
                // end it prematurely before we start processing the 
                // embedded protocol
                //
                // XXX: pdmlProto may be NULL for a sequence of embedded protos
                if (pdmlProto)
                {
                    int endPos = -1;

                    if (!attributes().value("pos").isEmpty())
                        endPos = attributes().value("pos").toString().toInt();

                    pdmlProto->prematureEndHandler(endPos, pbProto,
                            currentStream_);
                    pdmlProto->postProtocolHandler(pbProto, currentStream_);

                    StreamBase s;
                    s.protoDataCopyFrom(*currentStream_);
                    expPos_ = s.frameProtocolLength(0);
                }

                readProto();

                pdmlProto = NULL;
                pbProto = NULL;
            }
            else if (name() == "field")
            {
                if ((protoName == "fake-field-wrapper") &&
                        (attributes().value("name") == "tcp.segments"))
                {
                    skipElement();
                    qDebug("[skipping reassembled tcp segments]");

                    skipUntilEnd_ = true;
                    continue;
                }

                if (pdmlProto == NULL)
                {
                    pdmlProto = appendPdmlProto(protoName, &pbProto);

                    qDebug("%s: preProtocolHandler(expPos = %d)", 
                            protoName.toAscii().constData(), expPos_);
                    pdmlProto->preProtocolHandler(protoName, attributes(), 
                            expPos_, pbProto, currentStream_);
                }

                readField(pdmlProto, pbProto);
            }
            else 
                skipElement();
        }
    }

    // Close-off current protocol
    if (pdmlProto)
    {
        pdmlProto->postProtocolHandler(pbProto, currentStream_);
        freePdmlProtocol(pdmlProto);

        StreamBase s;
        s.protoDataCopyFrom(*currentStream_);
        expPos_ = s.frameProtocolLength(0);
    }
}

void PdmlReader::readField(PdmlDefaultProtocol *pdmlProto, 
        OstProto::Protocol *pbProto)
{
    Q_ASSERT(isStartElement() && name() == "field");

    // fields with "hide='yes'" are informational and should be skipped
    if (attributes().value("hide") == "yes")
    {
        skipElement();
        return;
    }

    QString fieldName = attributes().value("name").toString();

    qDebug("  fieldName:%s", fieldName.toAscii().constData());

    pdmlProto->fieldHandler(fieldName, attributes(), pbProto, currentStream_);

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "proto")
                readProto();
            else if (name() == "field")
                readField(pdmlProto, pbProto);
            else 
                skipElement();
        }
    }
}

void PdmlReader::appendHexDumpProto(int offset, int size)
{
    OstProto::Protocol *proto = currentStream_->add_protocol();
    OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

    proto->mutable_protocol_id()->set_id(
            OstProto::Protocol::kHexDumpFieldNumber);

    qDebug("filling in gap of %d bytes starting from %d", size, offset);
    hexDump->set_content(pktBuf_.constData() + offset, size);
    hexDump->set_pad_until_end(false);
}

PdmlDefaultProtocol* PdmlReader::appendPdmlProto(const QString &protoName,
        OstProto::Protocol **pbProto)
{
    PdmlDefaultProtocol* pdmlProto = allocPdmlProtocol(protoName);
    Q_ASSERT(pdmlProto != NULL);

    int protoId = pdmlProto->ostProtoId();

    if (protoId > 0) // Non-Base Class
    {
        OstProto::Protocol *proto = currentStream_->add_protocol();

        proto->mutable_protocol_id()->set_id(protoId);

        const google::protobuf::Reflection *msgRefl = proto->GetReflection();
        const google::protobuf::FieldDescriptor *fieldDesc = 
            msgRefl->FindKnownExtensionByNumber(protoId);

        // TODO: if !fDesc
        // init default values of all fields in protocol
        msgRefl->MutableMessage(proto, fieldDesc);

        *pbProto = proto;

        qDebug("%s: name = %s", __FUNCTION__, 
                protoName.toAscii().constData());
    }
    else
        *pbProto = NULL;

    return pdmlProto;
}


// ---------------------------------------------------------- //
// PdmlUnknownProtocol                                        //
// ---------------------------------------------------------- //

PdmlUnknownProtocol::PdmlUnknownProtocol()
{
    pdmlProtoName_ = "";
    ostProtoId_ = OstProto::Protocol::kHexDumpFieldNumber;

    endPos_ = expPos_ = -1;
}

PdmlDefaultProtocol* PdmlUnknownProtocol::createInstance()
{
    return new PdmlUnknownProtocol();
}

void PdmlUnknownProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    bool isOk;
    int size;
    int pos = attributes.value("pos").toString().toUInt(&isOk);
    if (!isOk)
    {
        if (expectedPos >= 0)
            expPos_ = pos = expectedPos;
        else
            goto _skip_pos_size_proc;
    }

    size = attributes.value("size").toString().toUInt(&isOk);
    if (!isOk)
        goto _skip_pos_size_proc;

    // If pos+size goes beyond the frame length, this is a "reassembled"
    // protocol and should be skipped
    if ((pos + size) > int(stream->core().frame_len()))
        goto _skip_pos_size_proc;

    expPos_ = pos;
    endPos_ = expPos_ + size;

_skip_pos_size_proc:
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);
    hexDump->set_pad_until_end(false);
}

void PdmlUnknownProtocol::prematureEndHandler(int pos, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    endPos_ = pos;
}

void PdmlUnknownProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = pbProto->MutableExtension(OstProto::hexDump);

    // Skipped field(s) at end? Pad with zero!
    if (endPos_ > expPos_)
    {
        QByteArray hexVal(endPos_ - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    qDebug("  hexdump: expPos_ = %d, endPos_ = %d\n", expPos_, endPos_); 

    // If empty for some reason, remove the protocol
    if (hexDump->content().size() == 0)
        stream->mutable_protocol()->RemoveLast();

    endPos_ = expPos_ = -1;
}

void PdmlUnknownProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = pbProto->MutableExtension(OstProto::hexDump);

    qDebug("  hexdump: %s, pos = %d, expPos_ = %d, endPos_ = %d\n", 
            name.toAscii().constData(), 
            pos, expPos_, endPos_); 

    // Skipped field? Pad with zero!
    if ((pos > expPos_) && (expPos_ < endPos_))
    {
        QByteArray hexVal(pos - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    if (pos == expPos_)
    {
        QByteArray hexVal = attributes.value("unmaskedvalue").isEmpty() ?
                QByteArray::fromHex(attributes.value("value").toString().toUtf8()) :
                QByteArray::fromHex(attributes.value("unmaskedvalue").toString().toUtf8());

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }
}


// ---------------------------------------------------------- //
// PdmlGenInfoProtocol                                        //
// ---------------------------------------------------------- //

PdmlGenInfoProtocol::PdmlGenInfoProtocol()
{
    pdmlProtoName_ = "geninfo";
}

PdmlDefaultProtocol* PdmlGenInfoProtocol::createInstance()
{
    return new PdmlGenInfoProtocol();
}

#if 0 // done in frame proto
void PdmlGenInfoProtocol::unknownFieldHandler(QString name, int pos, 
        int size, const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
{
    if (name == "len")
        stream->mutable_core()->set_frame_len(size+4); // TODO:check FCS
}
#endif

// ---------------------------------------------------------- //
// PdmlFrameProtocol                                          //
// ---------------------------------------------------------- //

PdmlFrameProtocol::PdmlFrameProtocol()
{
    pdmlProtoName_ = "frame";
}

PdmlDefaultProtocol* PdmlFrameProtocol::createInstance()
{
    return new PdmlFrameProtocol();
}

void PdmlFrameProtocol::unknownFieldHandler(QString name, int pos, int size, 
        const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "frame.len")
    {
        int len = -1;

        if (!attributes.value("show").isEmpty())
            len = attributes.value("show").toString().toInt();
        stream->mutable_core()->set_frame_len(len+4); // TODO:check FCS
    }
    else if (name == "frame.time_delta")
    {
        if (!attributes.value("show").isEmpty())
        {
            QString delta = attributes.value("show").toString();
            int decimal = delta.indexOf('.');
            
            if (decimal >= 0)
            {
                const uint kNsecsInSec = 1000000000;
                uint sec = delta.left(decimal).toUInt();
                uint nsec = delta.mid(decimal+1).toUInt();
                uint ipg = sec*kNsecsInSec + nsec;
                
                if (ipg)
                {
                    stream->mutable_control()->set_packets_per_sec(
                            kNsecsInSec/ipg);
                }

                qDebug("sec.nsec = %u.%u, ipg = %u", sec, nsec, ipg);
            }
        }
    }
}


// ---------------------------------------------------------- //
// PdmlSvlanProtocol                                          //
// ---------------------------------------------------------- //

PdmlSvlanProtocol::PdmlSvlanProtocol()
{
    pdmlProtoName_ = "ieee8021ad";
    ostProtoId_ = OstProto::Protocol::kSvlanFieldNumber;
}

PdmlDefaultProtocol* PdmlSvlanProtocol::createInstance()
{
    return new PdmlSvlanProtocol();
}

void PdmlSvlanProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);

    svlan->set_tpid(0x88a8);
    svlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes svlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the svlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kSvlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlSvlanProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if ((name == "ieee8021ad.id") || (name == "ieee8021ad.svid"))
    {
        bool isOk;
        OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ad.cvid")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kSvlanFieldNumber);

        OstProto::Vlan *svlan = proto->MutableExtension(OstProto::svlan);

        svlan->set_tpid(0x88a8);
        svlan->set_is_override_tpid(true);

        bool isOk;
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ah.etype") // yes 'ah' not 'ad' - not a typo!
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        bool isOk;
        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(attributes.value("value")
                .toString().toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
    }
}


// ---------------------------------------------------------- //
// PdmlVlanProtocol                                            //
// ---------------------------------------------------------- //

PdmlVlanProtocol::PdmlVlanProtocol()
{
    pdmlProtoName_ = "vlan";
    ostProtoId_ = OstProto::Protocol::kVlanFieldNumber;
}

PdmlDefaultProtocol* PdmlVlanProtocol::createInstance()
{
    return new PdmlVlanProtocol();
}

void PdmlVlanProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);

    vlan->set_tpid(0x8100);
    vlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes vlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the vlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kVlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlVlanProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "vlan.id")
    {
        bool isOk;
        OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        vlan->set_vlan_tag(tag);
    }
    else if (name == "vlan.etype")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        bool isOk;
        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(attributes.value("value")
                .toString().toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
    }
}


// ---------------------------------------------------------- //
// PdmlEthProtocol                                            //
// ---------------------------------------------------------- //

PdmlEthProtocol::PdmlEthProtocol()
{
    pdmlProtoName_ = "eth";
    ostProtoId_ = OstProto::Protocol::kMacFieldNumber;

    fieldMap_.insert("eth.dst", OstProto::Mac::kDstMacFieldNumber);
    fieldMap_.insert("eth.src", OstProto::Mac::kSrcMacFieldNumber);
}

PdmlDefaultProtocol* PdmlEthProtocol::createInstance()
{
    return new PdmlEthProtocol();
}

void PdmlEthProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "eth.type")
    {
        bool isOk;

        uint type = attributes.value("value").toString()
            .toUInt(&isOk, kBaseHex);

        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(type);
        eth2->set_is_override_type(true);
    }
    else if (name == "eth.len")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kDot3FieldNumber);

        OstProto::Dot3 *dot3 = proto->MutableExtension(OstProto::dot3);

        bool isOk;
        dot3->set_length(attributes.value("value").toString().toUInt(&isOk, kBaseHex));
        dot3->set_is_override_length(true);
    }
    else if (name == "eth.trailer")
    {
        QByteArray trailer = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());

        stream->mutable_core()->mutable_name()->append(trailer.constData(),
                trailer.size());
    }
    else if ((name == "eth.fcs") || 
            attributes.value("show").toString().startsWith("Frame check sequence"))
    {
        QByteArray trailer = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());

        stream->mutable_core()->mutable_name()->append(trailer.constData(),
                trailer.size());
    }
}


// ---------------------------------------------------------- //
// PdmlLlcProtocol                                            //
// ---------------------------------------------------------- //

PdmlLlcProtocol::PdmlLlcProtocol()
{
    pdmlProtoName_ = "llc";
    ostProtoId_ = OstProto::Protocol::kLlcFieldNumber;

    fieldMap_.insert("llc.dsap", OstProto::Llc::kDsapFieldNumber);
    fieldMap_.insert("llc.ssap", OstProto::Llc::kSsapFieldNumber);
    fieldMap_.insert("llc.control", OstProto::Llc::kCtlFieldNumber);
}

PdmlDefaultProtocol* PdmlLlcProtocol::createInstance()
{
    return new PdmlLlcProtocol();
}

void PdmlLlcProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "llc.oui")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kSnapFieldNumber);

        OstProto::Snap *snap = proto->MutableExtension(OstProto::snap);

        bool isOk;
        snap->set_oui(attributes.value("value").toString()
                .toUInt(&isOk, kBaseHex));
        snap->set_is_override_oui(true);
    }
    else if ((name == "llc.type") || (name.contains(QRegExp("llc\\..*pid"))))
    {
        OstProto::Snap *snap = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::snap);

        bool isOk;
        snap->set_type(attributes.value("value").toString()
                .toUInt(&isOk, kBaseHex));
        snap->set_is_override_type(true);
    }
}

void PdmlLlcProtocol::postProtocolHandler(OstProto::Protocol *pbProto, 
        OstProto::Stream *stream)
{
    OstProto::Llc *llc = pbProto->MutableExtension(OstProto::llc);

    llc->set_is_override_dsap(true);
    llc->set_is_override_ssap(true);
    llc->set_is_override_ctl(true);
}


// ---------------------------------------------------------- //
// PdmlArpProtocol                                            //
// ---------------------------------------------------------- //

PdmlArpProtocol::PdmlArpProtocol()
{
    pdmlProtoName_ = "arp";
    ostProtoId_ = OstProto::Protocol::kArpFieldNumber;

    fieldMap_.insert("arp.opcode", OstProto::Arp::kOpCodeFieldNumber);
    fieldMap_.insert("arp.src.hw_mac", OstProto::Arp::kSenderHwAddrFieldNumber);
    fieldMap_.insert("arp.src.proto_ipv4", 
            OstProto::Arp::kSenderProtoAddrFieldNumber);
    fieldMap_.insert("arp.dst.hw_mac", OstProto::Arp::kTargetHwAddrFieldNumber);
    fieldMap_.insert("arp.dst.proto_ipv4", 
            OstProto::Arp::kTargetProtoAddrFieldNumber);
}

PdmlDefaultProtocol* PdmlArpProtocol::createInstance()
{
    return new PdmlArpProtocol();
}


// ---------------------------------------------------------- //
// PdmlIp4Protocol                                            //
// ---------------------------------------------------------- //

PdmlIp4Protocol::PdmlIp4Protocol()
{
    pdmlProtoName_ = "ip";
    ostProtoId_ = OstProto::Protocol::kIp4FieldNumber;

    fieldMap_.insert("ip.version", OstProto::Ip4::kVerHdrlenFieldNumber);
    fieldMap_.insert("ip.dsfield", OstProto::Ip4::kTosFieldNumber);
    fieldMap_.insert("ip.len", OstProto::Ip4::kTotlenFieldNumber);
    fieldMap_.insert("ip.id", OstProto::Ip4::kIdFieldNumber);
    //fieldMap_.insert("ip.flags", OstProto::Ip4::kFlagsFieldNumber);
    fieldMap_.insert("ip.frag_offset", OstProto::Ip4::kFragOfsFieldNumber);
    fieldMap_.insert("ip.ttl", OstProto::Ip4::kTtlFieldNumber);
    fieldMap_.insert("ip.proto", OstProto::Ip4::kProtoFieldNumber);
    fieldMap_.insert("ip.checksum", OstProto::Ip4::kCksumFieldNumber);
    fieldMap_.insert("ip.src", OstProto::Ip4::kSrcIpFieldNumber);
    fieldMap_.insert("ip.dst", OstProto::Ip4::kDstIpFieldNumber);
}

PdmlDefaultProtocol* PdmlIp4Protocol::createInstance()
{
    return new PdmlIp4Protocol();
}

void PdmlIp4Protocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    bool isOk;

    if ((name == "ip.options") ||
            attributes.value("show").toString().startsWith("Options"))
    {
        options_ = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());
    }
    else if (name == "ip.flags")
    {
        OstProto::Ip4 *ip4 = pbProto->MutableExtension(OstProto::ip4);

        ip4->set_flags(attributes.value("value").toString().toUInt(&isOk, kBaseHex) >> 5);
    }
}

void PdmlIp4Protocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Ip4 *ip4 = pbProto->MutableExtension(OstProto::ip4);

    ip4->set_is_override_ver(true); // FIXME
    ip4->set_is_override_hdrlen(true); // FIXME
    ip4->set_is_override_totlen(true); // FIXME
    ip4->set_is_override_proto(true); // FIXME
    ip4->set_is_override_cksum(true); // FIXME

    if (options_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(options_.constData(), 
                options_.size());
        hexDump->set_pad_until_end(false);
        options_.resize(0);
    }
}

// ---------------------------------------------------------- //
// PdmlIp6Protocol                                            //
// ---------------------------------------------------------- //

PdmlIp6Protocol::PdmlIp6Protocol()
{
    pdmlProtoName_ = "ipv6";
    ostProtoId_ = OstProto::Protocol::kIp6FieldNumber;

    fieldMap_.insert("ipv6.version", OstProto::Ip6::kVersionFieldNumber);
    fieldMap_.insert("ipv6.class", OstProto::Ip6::kTrafficClassFieldNumber);
    fieldMap_.insert("ipv6.flow", OstProto::Ip6::kFlowLabelFieldNumber);
    fieldMap_.insert("ipv6.plen", OstProto::Ip6::kPayloadLengthFieldNumber);
    fieldMap_.insert("ipv6.nxt", OstProto::Ip6::kNextHeaderFieldNumber);
    fieldMap_.insert("ipv6.hlim", OstProto::Ip6::kHopLimitFieldNumber);

    // ipv6.src and ipv6.dst handled as unknown fields
}

PdmlDefaultProtocol* PdmlIp6Protocol::createInstance()
{
    return new PdmlIp6Protocol();
}

void PdmlIp6Protocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Protocol *pbProto,
            OstProto::Stream *stream)
{
    bool isOk;

    if (name == "ipv6.src")
    {
        OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_src_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_src_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "ipv6.dst")
    {
        OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_dst_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_dst_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
}

void PdmlIp6Protocol::postProtocolHandler(OstProto::Protocol *pbProto, 
        OstProto::Stream *stream)
{
    OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);

    ip6->set_is_override_version(true); // FIXME
    ip6->set_is_override_payload_length(true); // FIXME
    ip6->set_is_override_next_header(true); // FIXME
}

// ---------------------------------------------------------- //
// PdmlTcpProtocol                                            //
// ---------------------------------------------------------- //

PdmlTcpProtocol::PdmlTcpProtocol()
{
    pdmlProtoName_ = "tcp";
    ostProtoId_ = OstProto::Protocol::kTcpFieldNumber;

    fieldMap_.insert("tcp.srcport", OstProto::Tcp::kSrcPortFieldNumber);
    fieldMap_.insert("tcp.dstport", OstProto::Tcp::kDstPortFieldNumber);
    fieldMap_.insert("tcp.seq", OstProto::Tcp::kSeqNumFieldNumber);
    fieldMap_.insert("tcp.ack", OstProto::Tcp::kAckNumFieldNumber);
    fieldMap_.insert("tcp.hdr_len", OstProto::Tcp::kHdrlenRsvdFieldNumber);
    fieldMap_.insert("tcp.flags", OstProto::Tcp::kFlagsFieldNumber);
    fieldMap_.insert("tcp.window_size", OstProto::Tcp::kWindowFieldNumber);
    fieldMap_.insert("tcp.checksum", OstProto::Tcp::kCksumFieldNumber);
    fieldMap_.insert("tcp.urgent_pointer", OstProto::Tcp::kUrgPtrFieldNumber);
}

PdmlDefaultProtocol* PdmlTcpProtocol::createInstance()
{
    return new PdmlTcpProtocol();
}

void PdmlTcpProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Protocol *pbProto,
            OstProto::Stream *stream)
{
    if (name == "tcp.options")
        options_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
    else if (name == "")
    {
        if (attributes.value("show").toString().startsWith("TCP segment data"))
        {
            segmentData_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
            stream->mutable_core()->mutable_name()->insert(0, 
                    segmentData_.constData(), segmentData_.size());
        }
        else if (attributes.value("show").toString().startsWith("Acknowledgement number"))
        {
            bool isOk;
            OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

            tcp->set_ack_num(attributes.value("value").toString().toUInt(&isOk, kBaseHex)); 
        }
    }
}

void PdmlTcpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

    qDebug("Tcp: post\n");

    tcp->set_is_override_src_port(true); // FIXME
    tcp->set_is_override_dst_port(true); // FIXME
    tcp->set_is_override_hdrlen(true); // FIXME
    tcp->set_is_override_cksum(true); // FIXME

    if (options_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(options_.constData(), 
                options_.size());
        hexDump->set_pad_until_end(false);
        options_.resize(0);
    }

#if 0
    if (segmentData_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(segmentData_.constData(), 
                segmentData_.size());
        hexDump->set_pad_until_end(false);
        segmentData_.resize(0);
    }
#endif
}

// ---------------------------------------------------------- //
// PdmlUdpProtocol                                            //
// ---------------------------------------------------------- //

PdmlUdpProtocol::PdmlUdpProtocol()
{
    pdmlProtoName_ = "udp"; // OR udplite
    ostProtoId_ = OstProto::Protocol::kUdpFieldNumber;

    fieldMap_.insert("udp.srcport", OstProto::Udp::kSrcPortFieldNumber);
    fieldMap_.insert("udp.dstport", OstProto::Udp::kDstPortFieldNumber);
    fieldMap_.insert("udp.length", OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum_coverage", 
                     OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum", OstProto::Udp::kCksumFieldNumber);
}

PdmlDefaultProtocol* PdmlUdpProtocol::createInstance()
{
    return new PdmlUdpProtocol();
}

void PdmlUdpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Udp *udp = pbProto->MutableExtension(OstProto::udp);

    qDebug("Udp: post\n");

    udp->set_is_override_src_port(true);
    udp->set_is_override_dst_port(true);
    udp->set_is_override_totlen(true);
    udp->set_is_override_cksum(true);
}


// ---------------------------------------------------------- //
// PdmlTextProtocol                                            //
// ---------------------------------------------------------- //

PdmlTextProtocol::PdmlTextProtocol()
{
    pdmlProtoName_ = "text";
    ostProtoId_ = OstProto::Protocol::kTextProtocolFieldNumber;
}

PdmlDefaultProtocol* PdmlTextProtocol::createInstance()
{
    return new PdmlTextProtocol();
}

void PdmlTextProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    bool isOk;
    int size;
    int pos = attributes.value("pos").toString().toUInt(&isOk);

    if (!isOk)
    {
        if (expectedPos >= 0)
            expPos_ = pos = expectedPos;
        else
            goto _skip_pos_size_proc;
    }

    size = attributes.value("size").toString().toUInt(&isOk);
    if (!isOk)
        goto _skip_pos_size_proc;

    // If pos+size goes beyond the frame length, this is a "reassembled"
    // protocol and should be skipped
    if ((pos + size) > int(stream->core().frame_len()))
        goto _skip_pos_size_proc;

    expPos_ = pos;
    endPos_ = expPos_ + size;

_skip_pos_size_proc:
    qDebug("expPos_ = %d, endPos_ = %d", expPos_, endPos_);
    OstProto::TextProtocol *text = pbProto->MutableExtension(
            OstProto::textProtocol);

    text->set_port_num(0);
    text->set_eol(OstProto::TextProtocol::kCrLf);

    contentType_ = kUnknownContent;
}

void PdmlTextProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Protocol *pbProto,
            OstProto::Stream *stream)
{
    if (name == "data")
        contentType_ = kOtherContent;

    if (contentType_ == kOtherContent)
        return;

    if (contentType_ == kUnknownContent)
        contentType_ = kTextContent;

    // We process only kTextContent

    if (pos < expPos_)
        return;

    if ((pos + size) > endPos_)
        return;

    if (pos > expPos_)
    {   
        int gap = pos - expPos_;
        QString eol;
        QByteArray filler;
        OstProto::TextProtocol *text = pbProto->MutableExtension(
                OstProto::textProtocol);

        switch(text->eol())
        {
            case OstProto::TextProtocol::kCr: eol = "\r"; break;
            case OstProto::TextProtocol::kLf: eol = "\n"; break;
            case OstProto::TextProtocol::kCrLf: eol = "\r\n"; break;
            default: Q_ASSERT(false);
        }
        eol = "\n";

        for (int i = 0; i < gap; i++)
            filler += eol;

        filler.resize(gap);

        text->mutable_text()->append(filler.constData(), filler.size());
        expPos_ += gap;
    }

    if (attributes.value("show") == "HTTP chunked response")
        return;

    OstProto::TextProtocol *text = pbProto->MutableExtension(
            OstProto::textProtocol);

    QByteArray line = QByteArray::fromHex(
            attributes.value("value").toString().toUtf8());

    // Convert line endings to LF only - Qt requirement that TextProto honours
    line.replace("\r\n", "\n");
    line.replace("\r", "\n");

    text->mutable_text()->append(line.constData(), line.size());
    expPos_ += size;
}

void PdmlTextProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::TextProtocol *text = pbProto->MutableExtension(
            OstProto::textProtocol);

    // Empty Text Content - remove ourselves
    if (text->text().length() == 0)
        stream->mutable_protocol()->RemoveLast();

    expPos_ = endPos_ = -1;
    contentType_ = kUnknownContent;
}

