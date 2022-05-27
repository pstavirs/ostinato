/*
Copyright (C) 2021 Srivats P.

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

#include "grepdml.h"

#include "gre.pb.h"

PdmlGreProtocol::PdmlGreProtocol()
{
    ostProtoId_ = OstProto::Protocol::kGreFieldNumber;

    fieldMap_.insert("gre.proto", OstProto::Gre::kProtocolTypeFieldNumber);
    fieldMap_.insert("gre.checksum", OstProto::Gre::kChecksumFieldNumber);
    fieldMap_.insert("gre.offset", OstProto::Gre::kRsvd1FieldNumber);
    fieldMap_.insert("gre.key", OstProto::Gre::kKeyFieldNumber);
    fieldMap_.insert("gre.sequence_number", OstProto::Gre::kSequenceNumFieldNumber);
}

PdmlGreProtocol::~PdmlGreProtocol()
{
}

PdmlProtocol* PdmlGreProtocol::createInstance()
{
    return new PdmlGreProtocol();
}

void PdmlGreProtocol::postProtocolHandler(OstProto::Protocol* pbProto,
        OstProto::Stream* /*stream*/)
{
    OstProto::Gre *gre = pbProto->MutableExtension(OstProto::gre);

    qDebug("GRE: post");

    gre->set_is_override_checksum(overrideCksum_);
    return;
}

void PdmlGreProtocol::unknownFieldHandler(QString name,
        int /*pos*/, int /*size*/, const QXmlStreamAttributes& attributes,
        OstProto::Protocol* proto, OstProto::Stream* /*stream*/)
{
    if (name == "gre.flags_and_version") {
        bool isOk;
        OstProto::Gre *gre = proto->MutableExtension(OstProto::gre);
        quint16 flagsAndVersion = attributes.value("value")
                                        .toUInt(&isOk, kBaseHex);

        gre->set_flags(flagsAndVersion >> 12);
        gre->set_rsvd0((flagsAndVersion & 0x0FFF) >> 3);
        gre->set_version(flagsAndVersion & 0x0007);
    }
}
