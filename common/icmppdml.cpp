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

#include "icmppdml.h"

#include "icmp.pb.h"

PdmlIcmpProtocol::PdmlIcmpProtocol()
{
    ostProtoId_ = OstProto::Protocol::kIcmpFieldNumber;

    fieldMap_.insert("icmp.type", OstProto::Icmp::kTypeFieldNumber);
    fieldMap_.insert("icmp.code", OstProto::Icmp::kCodeFieldNumber);
    fieldMap_.insert("icmp.checksum", OstProto::Icmp::kChecksumFieldNumber);
    fieldMap_.insert("icmp.ident", OstProto::Icmp::kIdentifierFieldNumber);
    fieldMap_.insert("icmp.seq", OstProto::Icmp::kSequenceFieldNumber);

    fieldMap_.insert("icmpv6.type", OstProto::Icmp::kTypeFieldNumber);
    fieldMap_.insert("icmpv6.code", OstProto::Icmp::kCodeFieldNumber);
    fieldMap_.insert("icmpv6.checksum", OstProto::Icmp::kChecksumFieldNumber);
    fieldMap_.insert("icmpv6.echo.identifier", 
            OstProto::Icmp::kIdentifierFieldNumber);
    fieldMap_.insert("icmpv6.echo.sequence_number", 
            OstProto::Icmp::kSequenceFieldNumber);
}

PdmlProtocol* PdmlIcmpProtocol::createInstance()
{
    return new PdmlIcmpProtocol();
}

void PdmlIcmpProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if (name == "icmp")
        icmp->set_icmp_version(OstProto::Icmp::kIcmp4);
    else if (name == "icmpv6")
        icmp->set_icmp_version(OstProto::Icmp::kIcmp6);

    icmp->set_is_override_checksum(true);

    icmp->set_type(kIcmpInvalidType);
}

void PdmlIcmpProtocol::unknownFieldHandler(QString /*name*/, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if ((icmp->icmp_version() == OstProto::Icmp::kIcmp6)
            && (icmp->type() >= kIcmp6EchoRequest) 
            && (icmp->type() <= kIcmp6EchoReply))
    {
        QString addrHexStr = attributes.value("value").toString();

        // Wireshark 1.4.x does not have these as filterable fields
        if (attributes.value("show").toString().startsWith("ID"))
            icmp->set_identifier(addrHexStr.toUInt(&isOk, kBaseHex));
        else if (attributes.value("show").toString().startsWith("Sequence"))
            icmp->set_sequence(addrHexStr.toUInt(&isOk, kBaseHex));
    }
}

void PdmlIcmpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if (icmp->type() == kIcmpInvalidType)
        stream->mutable_protocol()->RemoveLast();
}

