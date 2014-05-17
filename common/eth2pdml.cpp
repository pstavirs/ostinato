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

#include "eth2pdml.h"

#include "dot3.pb.h"
#include "eth2.pb.h"
#include "mac.pb.h"
#include "vlan.pb.h"

PdmlEthProtocol::PdmlEthProtocol()
{
    ostProtoId_ = OstProto::Protocol::kMacFieldNumber;

    fieldMap_.insert("eth.dst", OstProto::Mac::kDstMacFieldNumber);
    fieldMap_.insert("eth.src", OstProto::Mac::kSrcMacFieldNumber);
}

PdmlProtocol* PdmlEthProtocol::createInstance()
{
    return new PdmlEthProtocol();
}

void PdmlEthProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
{
    if (name == "eth.vlan.tpid")
    {
        bool isOk;

        uint tpid = attributes.value("value").toString()
            .toUInt(&isOk, kBaseHex);

        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kVlanFieldNumber);

        OstProto::Vlan *vlan = proto->MutableExtension(OstProto::vlan);

        vlan->set_tpid(tpid);
        vlan->set_is_override_tpid(true);
    }
    else if (name == "eth.vlan.id")
    {
        bool isOk;

        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        OstProto::Vlan *vlan = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::vlan);

        vlan->set_vlan_tag(tag);
    }
    else if (name == "eth.type")
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
#if 0
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
#endif
}

