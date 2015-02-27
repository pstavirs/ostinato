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

#include "ip4pdml.h"

#include "hexdump.pb.h"
#include "ip4.pb.h"

PdmlIp4Protocol::PdmlIp4Protocol()
{
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

PdmlProtocol* PdmlIp4Protocol::createInstance()
{
    return new PdmlIp4Protocol();
}

void PdmlIp4Protocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
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

    ip4->set_is_override_ver(true);
    ip4->set_is_override_hdrlen(true);
    ip4->set_is_override_totlen(true);
    ip4->set_is_override_proto(true);
    ip4->set_is_override_cksum(true);

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

