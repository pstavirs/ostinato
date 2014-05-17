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

#include "vlanpdml.h"

#include "eth2.pb.h"
#include "vlan.pb.h"

PdmlVlanProtocol::PdmlVlanProtocol()
{
    ostProtoId_ = OstProto::Protocol::kVlanFieldNumber;
}

PdmlProtocol* PdmlVlanProtocol::createInstance()
{
    return new PdmlVlanProtocol();
}

void PdmlVlanProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);

    vlan->set_tpid(0x8100);
    vlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes vlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the vlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kVlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlVlanProtocol::unknownFieldHandler(QString name, int /*pos*/,
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "vlan.id")
    {
        bool isOk;
        OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        vlan->set_vlan_tag(tag);
    }
    else if (name == "vlan.etype")
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

