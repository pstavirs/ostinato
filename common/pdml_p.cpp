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

#include "mac.pb.h"
#include "eth2.pb.h"
#include "dot3.pb.h"
#include "hexdump.pb.h"
#include "ip4.pb.h"
#include "ip6.pb.h"
#include "tcp.pb.h"

#include <google/protobuf/descriptor.h>

#include <QMessageBox>

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
        int /*expectedPos*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::prematureEndHandler(int pos, OstProto::Stream *stream)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::postProtocolHandler(OstProto::Stream *stream)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::unknownFieldHandler(QString name, 
        int pos, int size, const QXmlStreamAttributes &attributes, 
        OstProto::Stream *stream)
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

    factory_.insert("hexdump", PdmlUnknownProtocol::createInstance);
    factory_.insert("geninfo", PdmlGenInfoProtocol::createInstance);
    factory_.insert("frame", PdmlFrameProtocol::createInstance);
    factory_.insert("eth",PdmlEthProtocol::createInstance);
    factory_.insert("ip",PdmlIp4Protocol::createInstance);
    factory_.insert("ipv6",PdmlIp6Protocol::createInstance);
    factory_.insert("tcp",PdmlTcpProtocol::createInstance);
}

PdmlReader::~PdmlReader()
{
}

bool PdmlReader::read(QIODevice *device, PcapFileFormat *pcap)
{
    setDevice(device);
    pcap_ = pcap;
    packetCount_ = 0;

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

    if (error())
    {
        qDebug("Line %lld", lineNumber());
        qDebug("Col %lld", columnNumber());
        qDebug("%s", errorString().toAscii().constData()); // FIXME
        return false;
    }
    return true;
}

// TODO: use a temp pool to avoid a lot of new/delete
PdmlDefaultProtocol* PdmlReader::allocPdmlProtocol(QString protoName)
{
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

    QString protoName = attributes().value("name").toString();

    if (protoName.isEmpty() || (protoName == "expert"))
        return true;
    else
        return false;
}

void PdmlReader::readUnexpectedElement()
{
    Q_ASSERT(isStartElement());

    // XXX: add to 'log'
    qDebug("unexpected element - <%s>; skipping ...", 
                    name().toString().toAscii().constData());
    while (!atEnd())
    {
        readNext();
        if (isEndElement())
            break;

        if (isStartElement())
            readUnexpectedElement();
    }
}

void PdmlReader::skipElement()
{
    Q_ASSERT(isStartElement());

    // XXX: add to 'log'
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
                readUnexpectedElement();
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

    // Set to a high number; will get reset to correct during parse
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
                readUnexpectedElement();
        }
    }
    // BAD Hack for TCP Segments
    if (currentStream_->core().name().size())
#if 0
    {
        OstProto::Protocol *proto = currentStream_->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        qDebug("Adding TCP Segment Data/FCS etc of size %d",
                currentStream_->core().name().size());

        hexDump->set_content(currentStream_->core().name());
        hexDump->set_pad_until_end(false);
        currentStream_->mutable_core()->set_name("");

        expPos_ += hexDump->content().size();
    }
#else
        currentStream_->mutable_core()->set_name("");
#endif

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
    if (prevStream_)
        prevStream_->mutable_control()->CopyFrom(currentStream_->control());
}

void PdmlReader::readProto()
{
    PdmlDefaultProtocol *pdmlProto = NULL;
    google::protobuf::Message *pbProto = NULL;

    Q_ASSERT(isStartElement() && name() == "proto");

    QString protoName = attributes().value("name").toString();
    int pos = -1;
    int size = -1;

    if (!attributes().value("pos").isEmpty())
        pos = attributes().value("pos").toString().toInt();
    if (!attributes().value("size").isEmpty())
        size = attributes().value("size").toString().toInt();

    qDebug("proto: %s, pos = %d, expPos_ = %d", 
            protoName.toAscii().constData(), pos, expPos_);

    // This is a heuristic to skip protocols which are not part of
    // this frame, but of a reassembled segment spanning several frames
    //   1. Proto starting pos is 0, but we already seen some protocols
    //   2. Protocol Size exceeds frame length
    if (((pos == 0) && (currentStream_->protocol_size() > 0))
        || ((pos + size) > int(currentStream_->core().frame_len())))
    {
        qDebug("(skipped)");
        skipElement();
        return;
    }

#if 1
    if (protoName.isEmpty() || (protoName == "expert"))
    {
        skipElement();
        return;
    }
#endif

    // detect overlaps or gaps between subsequent protocols and "fill-in"
    // with a "hexdump" from the pcap
    if (pos >=0 && pcap_)
    {
        if (pos > expPos_)
        {
            OstProto::Protocol *proto = currentStream_->add_protocol();
            OstProto::HexDump *hexDump = proto->MutableExtension(
                    OstProto::hexDump);

            proto->mutable_protocol_id()->set_id(
                    OstProto::Protocol::kHexDumpFieldNumber);

            qDebug("filling in gap of %d bytes starting from %d",
                    pos - expPos_, expPos_);
            hexDump->set_content(pktBuf_.constData() + expPos_, pos - expPos_);
            hexDump->set_pad_until_end(false);

            expPos_ = pos;
        } 
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
            OstProto::Protocol *proto = currentStream_->add_protocol();
            OstProto::HexDump *hexDump = proto->MutableExtension(
                    OstProto::hexDump);

            proto->mutable_protocol_id()->set_id(
                    OstProto::Protocol::kHexDumpFieldNumber);

            qDebug("missing bytes - filling in %d bytes staring from %d",
                    size, pos);
            hexDump->set_content(pktBuf_.constData() + pos, size);
            hexDump->set_pad_until_end(false);

            skipElement();

            expPos_ += size;
            return;
        }
    }

    pdmlProto = allocPdmlProtocol(protoName);
    Q_ASSERT(pdmlProto != NULL);

    int protoId = pdmlProto->ostProtoId();

    // Non PdmlDefaultProtocol
    if (protoId > 0)
    {
        OstProto::Protocol *proto = currentStream_->add_protocol();

        proto->mutable_protocol_id()->set_id(protoId);

        const google::protobuf::Reflection *msgRefl = proto->GetReflection();
        const google::protobuf::FieldDescriptor *fieldDesc = 
            msgRefl->FindKnownExtensionByNumber(protoId);

        // TODO: if !fDesc
        // init default values of all fields in protocol
        pbProto = msgRefl->MutableMessage(proto, fieldDesc);

    }

    pdmlProto->preProtocolHandler(protoName, attributes(), expPos_, 
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
                int endPos = -1;
                // an embedded proto
                qDebug("embedded proto: %s\n", attributes().value("name")
                        .toString().toAscii().constData());
                if (isDontCareProto())
                {
                    skipElement();
                    continue;
                }

                if (!attributes().value("pos").isEmpty())
                    endPos = attributes().value("pos").toString().toInt();


                // pdmlProto may be NULL for a sequence of embedded protos
                if (pdmlProto)
                {
                    pdmlProto->prematureEndHandler(endPos, currentStream_);
                    pdmlProto->postProtocolHandler(currentStream_);

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
                    pdmlProto = allocPdmlProtocol(protoName);
                    protoId = pdmlProto->ostProtoId();

                    // Non PdmlDefaultProtocol
                    if (protoId > 0)
                    {
                        OstProto::Protocol *proto = currentStream_->add_protocol();

                        proto->mutable_protocol_id()->set_id(protoId);

                        const google::protobuf::Reflection *msgRefl = proto->GetReflection();
                        const google::protobuf::FieldDescriptor *fieldDesc = 
                            msgRefl->FindKnownExtensionByNumber(protoId);

                        // TODO: if !fDesc
                        // init default values of all fields in protocol
                        pbProto = msgRefl->MutableMessage(proto, fieldDesc);

                    }
                    pdmlProto->preProtocolHandler(protoName, attributes(), 
                            expPos_, currentStream_);
                }
                readField(pdmlProto, pbProto);
            }
            else 
                readUnexpectedElement();
        }
    }

    if (pdmlProto)
    {
        pdmlProto->postProtocolHandler(currentStream_);
        freePdmlProtocol(pdmlProto);

        StreamBase s;
        s.protoDataCopyFrom(*currentStream_);
        expPos_ = s.frameProtocolLength(0);
    }
}

void PdmlReader::readField(PdmlDefaultProtocol *pdmlProto, 
        google::protobuf::Message *pbProto)
{
    Q_ASSERT(isStartElement() && name() == "field");

    // fields with "hide='yes'" are informational and should be skipped
    if (attributes().value("hide") == "yes")
    {
        skipElement();
        return;
    }

    QString fieldName = attributes().value("name").toString();
    QString valueHexStr = attributes().value("value").toString();
    int pos = -1;
    int size = -1;

    if (!attributes().value("pos").isEmpty())
        pos = attributes().value("pos").toString().toInt();
    if (!attributes().value("size").isEmpty())
        size = attributes().value("size").toString().toInt();

    qDebug("\tfieldName:%s, pos:%d, size:%d value:%s",
            fieldName.toAscii().constData(), 
            pos, 
            size, 
            valueHexStr.toAscii().constData());

    if (pdmlProto->hasField(fieldName))
    {
        int fieldId = pdmlProto->fieldId(fieldName);
        const google::protobuf::Descriptor *msgDesc = 
            pbProto->GetDescriptor();
        const google::protobuf::FieldDescriptor *fieldDesc = 
            msgDesc->FindFieldByNumber(fieldId);
        const google::protobuf::Reflection *msgRefl = 
            pbProto->GetReflection();

        bool isOk;

        switch(fieldDesc->cpp_type())
        {
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // TODO
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            msgRefl->SetUInt32(pbProto, fieldDesc, 
                    valueHexStr.toUInt(&isOk, kBaseHex));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            msgRefl->SetUInt64(pbProto, fieldDesc, 
                    valueHexStr.toULongLong(&isOk, kBaseHex));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            QByteArray hexVal = QByteArray::fromHex(valueHexStr.toUtf8());
            std::string str(hexVal.constData(), hexVal.size());
            msgRefl->SetString(pbProto, fieldDesc, str);
            break;
        }
        default:
            qDebug("%s: unhandled cpptype = %d", __FUNCTION__, 
                    fieldDesc->cpp_type());
        }
    }
    else
    {
        pdmlProto->unknownFieldHandler(fieldName, pos, size, attributes(), 
                currentStream_);
    }

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "proto")
            {
                if (isDontCareProto())
                {
                    skipElement();
                    continue;
                }
                readProto();
            }
            else if (name() == "field")
                readField(pdmlProto, pbProto);
            else 
                readUnexpectedElement();
        }
    }
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
        const QXmlStreamAttributes &attributes,
        int expectedPos, OstProto::Stream *stream)
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

void PdmlUnknownProtocol::prematureEndHandler(int pos, OstProto::Stream *stream)
{
    endPos_ = pos;
}

void PdmlUnknownProtocol::postProtocolHandler(OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);

    // Skipped field(s) at end? Pad with zero!
    if (endPos_ > expPos_)
    {
        QByteArray hexVal(endPos_ - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    qDebug("  hexdump: expPos_ = %d, endPos_ = %d\n", expPos_, endPos_); 
    //Q_ASSERT(expPos_ == endPos_);

    hexDump->set_pad_until_end(false);

    // If empty for some reason, remove the protocol
    if (hexDump->content().size() == 0)
        stream->mutable_protocol()->RemoveLast();

    endPos_ = expPos_ = -1;
}

void PdmlUnknownProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);

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

    if ((pos == expPos_) /*&& (pos < endPos_)*/)
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

void PdmlFrameProtocol::unknownFieldHandler(QString name, int pos, 
        int size, const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
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
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
{
    if (name == "eth.type")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        bool isOk;
        eth2->set_type(attributes.value("value").toString().toUInt(&isOk, kBaseHex));
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
// PdmlIp4Protocol                                            //
// ---------------------------------------------------------- //

PdmlIp4Protocol::PdmlIp4Protocol()
{
    pdmlProtoName_ = "ip";
    ostProtoId_ = OstProto::Protocol::kIp4FieldNumber;

    fieldMap_.insert("ip.version", 5);
    fieldMap_.insert("ip.dsfield", 6);
    fieldMap_.insert("ip.len", 7);
    fieldMap_.insert("ip.id", 8);
    //fieldMap_.insert("ip.flags", 9);
    fieldMap_.insert("ip.frag_offset", 10);
    fieldMap_.insert("ip.ttl", 11);
    fieldMap_.insert("ip.proto", 12);
    fieldMap_.insert("ip.checksum", 13);
    fieldMap_.insert("ip.src", 14);
    fieldMap_.insert("ip.dst", 18);
}

PdmlDefaultProtocol* PdmlIp4Protocol::createInstance()
{
    return new PdmlIp4Protocol();
}

void PdmlIp4Protocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
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
        OstProto::Ip4 *ip4 = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::ip4);

        ip4->set_flags(attributes.value("value").toString().toUInt(&isOk, kBaseHex) >> 5);
    }
}

void PdmlIp4Protocol::postProtocolHandler(OstProto::Stream *stream)
{
    OstProto::Ip4 *ip4 = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::ip4);

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
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
{
    bool isOk;

    if (name == "ipv6.src")
    {
        OstProto::Ip6 *ip6 = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_src_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_src_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "ipv6.dst")
    {
        OstProto::Ip6 *ip6 = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_dst_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_dst_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
}

void PdmlIp6Protocol::postProtocolHandler(OstProto::Stream *stream)
{
    OstProto::Ip6 *ip6 = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::ip6);

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
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
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
            OstProto::Tcp *tcp = stream->mutable_protocol(
                    stream->protocol_size()-1)->MutableExtension(OstProto::tcp);

            tcp->set_ack_num(attributes.value("value").toString().toUInt(&isOk, kBaseHex)); 
        }
    }
}

void PdmlTcpProtocol::postProtocolHandler(OstProto::Stream *stream)
{
    OstProto::Tcp *tcp = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::tcp);

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

