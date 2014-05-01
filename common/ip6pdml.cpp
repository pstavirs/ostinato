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

#include "ip6pdml.h"

#include "ip6.pb.h"

PdmlIp6Protocol::PdmlIp6Protocol()
{
    ostProtoId_ = OstProto::Protocol::kIp6FieldNumber;

    fieldMap_.insert("ipv6.version", OstProto::Ip6::kVersionFieldNumber);
    fieldMap_.insert("ipv6.class", OstProto::Ip6::kTrafficClassFieldNumber);
    fieldMap_.insert("ipv6.flow", OstProto::Ip6::kFlowLabelFieldNumber);
    fieldMap_.insert("ipv6.plen", OstProto::Ip6::kPayloadLengthFieldNumber);
    fieldMap_.insert("ipv6.nxt", OstProto::Ip6::kNextHeaderFieldNumber);
    fieldMap_.insert("ipv6.hlim", OstProto::Ip6::kHopLimitFieldNumber);

    // ipv6.src and ipv6.dst handled as unknown fields
}

PdmlProtocol* PdmlIp6Protocol::createInstance()
{
    return new PdmlIp6Protocol();
}

void PdmlIp6Protocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
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
        OstProto::Stream* /*stream*/)
{
    OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);

    ip6->set_is_override_version(true);
    ip6->set_is_override_payload_length(true);
    ip6->set_is_override_next_header(true);
}

