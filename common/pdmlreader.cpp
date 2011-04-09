/*
Copyright (C) 2011 Srivats P.

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

#include "pdmlreader.h"

#include "abstractprotocol.h"
#include "hexdump.pb.h"
#include "pcapfileformat.h"
#include "streambase.h"

#include "pdml_p.h" // TODO: Remove

PdmlReader::PdmlReader(OstProto::StreamConfigList *streams)
{
    //gPdmlReader = this;
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
    factory_.insert("icmp", PdmlIcmpProtocol::createInstance);
    factory_.insert("icmpv6", PdmlIcmp6Protocol::createInstance);
    factory_.insert("igmp", PdmlIgmpProtocol::createInstance);
    factory_.insert("ieee8021ad", PdmlSvlanProtocol::createInstance);
    factory_.insert("imap", PdmlTextProtocol::createInstance);
    factory_.insert("ip", PdmlIp4Protocol::createInstance);
    factory_.insert("ipv6", PdmlIp6Protocol::createInstance);
    factory_.insert("llc", PdmlLlcProtocol::createInstance);
    factory_.insert("nntp", PdmlTextProtocol::createInstance);
    factory_.insert("pop", PdmlTextProtocol::createInstance);
    factory_.insert("rtsp", PdmlTextProtocol::createInstance);
    factory_.insert("sdp", PdmlTextProtocol::createInstance);
    factory_.insert("sip", PdmlTextProtocol::createInstance);
    factory_.insert("smtp", PdmlTextProtocol::createInstance);
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
PdmlProtocol* PdmlReader::allocPdmlProtocol(QString protoName)
{
    // If protoName is not known, we use a hexdump
    if (!factory_.contains(protoName))
        protoName = "hexdump";

    return (*(factory_.value(protoName)))();
}

void PdmlReader::freePdmlProtocol(PdmlProtocol *proto)
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
    PdmlProtocol *pdmlProto = NULL;
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

void PdmlReader::readField(PdmlProtocol *pdmlProto, 
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
            {
                // Since we are in the midst of processing a protocol, we
                // end it prematurely before we start processing the 
                // embedded protocol
                //
                int endPos = -1;

                if (!attributes().value("pos").isEmpty())
                    endPos = attributes().value("pos").toString().toInt();

                pdmlProto->prematureEndHandler(endPos, pbProto,
                        currentStream_);
                pdmlProto->postProtocolHandler(pbProto, currentStream_);

                StreamBase s;
                s.protoDataCopyFrom(*currentStream_);
                expPos_ = s.frameProtocolLength(0);

                readProto();
            }
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

PdmlProtocol* PdmlReader::appendPdmlProto(const QString &protoName,
        OstProto::Protocol **pbProto)
{
    PdmlProtocol* pdmlProto = allocPdmlProtocol(protoName);
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
