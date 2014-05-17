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

#include "tcppdml.h"

#include "hexdump.pb.h"
#include "tcp.pb.h"

PdmlTcpProtocol::PdmlTcpProtocol()
{
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

PdmlProtocol* PdmlTcpProtocol::createInstance()
{
    return new PdmlTcpProtocol();
}

void PdmlTcpProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    if (name == "tcp.options")
        options_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
    else if (name == "")
    {
        if (attributes.value("show").toString().startsWith("Acknowledgement number"))
        {
            bool isOk;
            OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

            tcp->set_ack_num(attributes.value("value").toString().toUInt(&isOk, kBaseHex)); 
        }
#if 0
        else if (attributes.value("show").toString().startsWith("TCP segment data"))
        {
            segmentData_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
            stream->mutable_core()->mutable_name()->insert(0, 
                    segmentData_.constData(), segmentData_.size());
        }
#endif
    }
}

void PdmlTcpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

    qDebug("Tcp: post\n");

    tcp->set_is_override_src_port(true);
    tcp->set_is_override_dst_port(true);
    tcp->set_is_override_hdrlen(true);
    tcp->set_is_override_cksum(true);

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

