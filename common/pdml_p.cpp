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

#include "mac.pb.h"
#include "eth2.pb.h"
#include "ip4.pb.h"
#include "hexdump.pb.h"

#include <google/protobuf/descriptor.h>

#include <QMessageBox>

#include <string>

const int kBaseHex = 16;

PdmlDefaultProtocol::PdmlDefaultProtocol()
{
    ostProtoId_ = -1;
}

PdmlDefaultProtocol::~PdmlDefaultProtocol()
{
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

void PdmlDefaultProtocol::preProtocolHandler(QString name, 
        const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::postProtocolHandler(OstProto::Stream *stream)
{
    return; // do nothing!
}

void PdmlDefaultProtocol::unknownFieldHandler(QString name, 
        int pos, int size, const QXmlAttributes &attributes, 
        OstProto::Stream *stream)
{
    return; // do nothing!
}


// ---------------------------------------------------------- //
// PdmlParser
// ---------------------------------------------------------- //
PdmlParser::PdmlParser(OstProto::StreamConfigList *streams) 
{ 
    skipUntilEndOfPacket_ = false;
    skipCount_ = 0;

    streams_ =  streams;

    protocolMap_.insert("unknown", new PdmlUnknownProtocol());
    protocolMap_.insert("geninfo", new PdmlGenInfoProtocol());
    protocolMap_.insert("frame", new PdmlFrameProtocol());
    protocolMap_.insert("eth", new PdmlEthProtocol());
    protocolMap_.insert("ip", new PdmlIp4Protocol());
}

PdmlParser::~PdmlParser()
{
    // TODO: free protocolMap_.values()
}

bool PdmlParser::startElement(const QString & /* namespaceURI */,
    const QString & /* localName */,
    const QString &qName,
    const QXmlAttributes &attributes)
{
    qDebug("%s (%s)", __FUNCTION__, qName.toAscii().constData());

    if (skipUntilEndOfPacket_)
    {
        Q_ASSERT(skipCount_ == 0);
        goto _exit;
    }

    if (skipCount_)
    {
        skipCount_++;
        goto _exit;
    }

    if (qName == "pdml") 
    {
        packetCount_ = 0;
    }
    else if (qName == "packet") 
    {
        Q_ASSERT(skipUntilEndOfPacket_ == false);

        // XXX: For now, each packet is converted to a stream
        currentStream_ = streams_->add_stream();
        currentStream_->mutable_stream_id()->set_id(packetCount_);
        currentStream_->mutable_core()->set_is_enabled(true);

        qDebug("packetCount_ = %d\n", packetCount_);
    }
    else if (qName == "proto")
    {
        QString protoName = attributes.value("name");
        if (protoName.isEmpty()
            || (protoName == "expert"))
        {
            skipCount_++;
            goto _exit;
        }

        if (protoName == "fake-field-wrapper")
        {
            skipUntilEndOfPacket_ = true;
            goto _exit;
        }

        if (!protocolMap_.contains(protoName))
            protoName = "unknown"; // FIXME: change to Ost:Hexdump

        currentPdmlProtocol_ = protocolMap_.value(protoName);

        Q_ASSERT(currentPdmlProtocol_ != NULL);

        int protoId = currentPdmlProtocol_->ostProtoId();

        currentPdmlProtocol_->preProtocolHandler(protoName, attributes,
                currentStream_);

        if (protoId > 0)
        {
            OstProto::Protocol *proto = currentStream_->add_protocol();

            proto->mutable_protocol_id()->set_id(protoId);

            const google::protobuf::Reflection *msgRefl = 
                proto->GetReflection();

            const google::protobuf::FieldDescriptor *fDesc = 
                msgRefl->FindKnownExtensionByNumber(protoId);

            // TODO: if !fDesc
            // init default values of all fields in protocol
            currentProtocolMsg_ = msgRefl->MutableMessage(proto, fDesc);
        }
    }
    else if (qName == "field")
    {
        // fields with "hide='yes'" are informational and should be skipped
        if (attributes.value("hide") == "yes")
        {
            skipCount_++;
            goto _exit;
        }

        QString name = attributes.value("name");
        int pos = attributes.value("pos").toInt();
        int size = attributes.value("size").toInt();
        QString valueStr = attributes.value("value");

        // fields with no name are analysis and should be skipped
        if (name.isEmpty())
        {
            skipCount_++;
            goto _exit;
        }

        qDebug("\tname:%s, pos:%d, size:%d value:%s",
                name.toAscii().constData(), 
                pos, 
                size, 
                valueStr.toAscii().constData());

        if (!currentPdmlProtocol_->hasField(name))
        {
            currentPdmlProtocol_->unknownFieldHandler(name, pos, size, 
                    attributes, currentStream_);
            goto _exit;
        }

        // TODO
        int fId = currentPdmlProtocol_->fieldId(name);
        const google::protobuf::Descriptor *msgDesc = 
            currentProtocolMsg_->GetDescriptor();
        const google::protobuf::FieldDescriptor *fDesc = 
            msgDesc->FindFieldByNumber(fId);
        const google::protobuf::Reflection *msgRefl = 
            currentProtocolMsg_->GetReflection();

        bool isOk;

        switch(fDesc->cpp_type())
        {
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // TODO
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            msgRefl->SetUInt32(currentProtocolMsg_, fDesc, 
                    valueStr.toUInt(&isOk, kBaseHex));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            msgRefl->SetUInt64(currentProtocolMsg_, fDesc, 
                    valueStr.toULongLong(&isOk, kBaseHex));
        default:
            qDebug("%s: unhandled cpptype = %d", __FUNCTION__, 
                    fDesc->cpp_type());
        }
    }

_exit:
    return true;

}

bool PdmlParser::characters(const QString &str)
{
    //qDebug("%s (%s)", __FUNCTION__, str.toAscii().constData());
    //_currentText += str;
    return true;
}

bool PdmlParser::endElement(const QString & /* namespaceURI */,
                            const QString & /* localName */,
                            const QString &qName)
{
    qDebug("%s (%s)", __FUNCTION__, qName.toAscii().constData());

    if (qName == "packet") 
    {
        packetCount_++;
        if (skipUntilEndOfPacket_)
            skipUntilEndOfPacket_ = false;

        goto _exit;
    }

    if (skipUntilEndOfPacket_)
    {
        Q_ASSERT(skipCount_ == 0);
        goto _exit;
    }

    if (skipCount_)
    {
        skipCount_--;
        goto _exit;
    }

    if (qName == "proto")
    {
        currentPdmlProtocol_->postProtocolHandler(currentStream_);
    }
    else if (qName == "field")
    {
    }

_exit:
    return true;
}

bool PdmlParser::fatalError(const QXmlParseException &exception)
{
    QString extra;

    qDebug("%s", __FUNCTION__);
#if 0
    if (exception.message() == "tag mismatch" && lastElement == "fieldData")
        extra = "\nAre you using an old version of Wireshark? If so, try using a newer version. Alternatively, view the packet dump decode in Wireshark by clicking the \"External\" button.";
#endif

    QMessageBox::warning(0, QObject::tr("PDML Parser"),
                         QObject::tr("XML parse error for packet %1 "
                            "at line %2, column %3:\n    %4\n%5")
                         .arg(packetCount_+1)
                         .arg(exception.lineNumber())
                         .arg(exception.columnNumber())
                         .arg(exception.message())
                         .arg(extra));
    return false;
}


// ---------------------------------------------------------- //
// PdmlUnknownProtocol                                        //
// ---------------------------------------------------------- //

PdmlUnknownProtocol::PdmlUnknownProtocol()
{
    pdmlProtoName_ = "OST:HexDump";
    ostProtoId_ = OstProto::Protocol::kHexDumpFieldNumber;

    endPos_ = expPos_ = -1;
}

void PdmlUnknownProtocol::preProtocolHandler(QString name, 
        const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    bool isOk;
    int pos = attributes.value("pos").toUInt(&isOk);
    Q_ASSERT(isOk);

    int size = attributes.value("size").toUInt(&isOk);
    Q_ASSERT(isOk);

    expPos_ = pos;
    endPos_ = expPos_ + size;
}

void PdmlUnknownProtocol::postProtocolHandler(OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);

    // Skipped field? Pad with zero!
    if (endPos_ > expPos_)
    {
        QByteArray hexVal(endPos_ - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    Q_ASSERT(expPos_ == endPos_);

    hexDump->set_pad_until_end(false);
    expPos_ = -1;
}

void PdmlUnknownProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);

    qDebug("%s: %s, pos = %d, expPos_ = %d\n", __FUNCTION__, 
            name.toAscii().constData(), pos, expPos_); 

    // Skipped field? Pad with zero!
    if (pos > expPos_)
    {
        QByteArray hexVal(pos - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    if (pos == expPos_)
    {
        QByteArray hexVal = 
                QByteArray::fromHex(attributes.value("value").toUtf8());

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

void PdmlGenInfoProtocol::unknownFieldHandler(QString name, int pos, 
        int size, const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    stream->mutable_core()->set_frame_len(size+4); // TODO:check FCS
}

// ---------------------------------------------------------- //
// PdmlFrameProtocol                                          //
// ---------------------------------------------------------- //

PdmlFrameProtocol::PdmlFrameProtocol()
{
    pdmlProtoName_ = "frame";
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

void PdmlEthProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    if (name == "eth.type")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        bool isOk;
        eth2->set_type(attributes.value("value").toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
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

void PdmlIp4Protocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream)
{
    bool isOk;

    if (name == "ip.flags")
    {
        OstProto::Ip4 *ip4 = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::ip4);

        ip4->set_flags(attributes.value("value").toUInt(&isOk, kBaseHex) >> 5);
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
}

