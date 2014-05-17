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

#include "llcpdml.h"

#include "llc.pb.h"
#include "snap.pb.h"

#include <QRegExp>

PdmlLlcProtocol::PdmlLlcProtocol()
{
    ostProtoId_ = OstProto::Protocol::kLlcFieldNumber;

    fieldMap_.insert("llc.dsap", OstProto::Llc::kDsapFieldNumber);
    fieldMap_.insert("llc.ssap", OstProto::Llc::kSsapFieldNumber);
    fieldMap_.insert("llc.control", OstProto::Llc::kCtlFieldNumber);
}

PdmlProtocol* PdmlLlcProtocol::createInstance()
{
    return new PdmlLlcProtocol();
}

void PdmlLlcProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
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
        OstProto::Stream* /*stream*/)
{
    OstProto::Llc *llc = pbProto->MutableExtension(OstProto::llc);

    llc->set_is_override_dsap(true);
    llc->set_is_override_ssap(true);
    llc->set_is_override_ctl(true);
}

