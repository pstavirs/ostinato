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

#include "svlanpdml.h"

#include "eth2.pb.h"
#include "svlan.pb.h"

PdmlSvlanProtocol::PdmlSvlanProtocol()
{
    ostProtoId_ = OstProto::Protocol::kSvlanFieldNumber;
}

PdmlProtocol* PdmlSvlanProtocol::createInstance()
{
    return new PdmlSvlanProtocol();
}

void PdmlSvlanProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);

    svlan->set_tpid(0x88a8);
    svlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes svlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the svlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kSvlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlSvlanProtocol::unknownFieldHandler(QString name, int /*pos*/,
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if ((name == "ieee8021ad.id") || (name == "ieee8021ad.svid"))
    {
        bool isOk;
        OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ad.cvid")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kSvlanFieldNumber);

        OstProto::Vlan *svlan = proto->MutableExtension(OstProto::svlan);

        svlan->set_tpid(0x88a8);
        svlan->set_is_override_tpid(true);

        bool isOk;
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ah.etype") // yes 'ah' not 'ad' - not a typo!
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        bool isOk;
        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(attributes.value("value")
                .toString().toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
    }
}

