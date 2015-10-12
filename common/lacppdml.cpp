/*
Copyright (C) 2014 Marchuk S.

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

This module is developed by PLVision <developers@plvision.eu> company
*/

#include "lacppdml.h"

#include "lacp.pb.h"
#include "hexdump.pb.h"

#define ACTOR_RESERVED_POS 33
#define PARTNER_RESERVED_POS 53
#define BASE_DEC 10
#define BASE_HEX 16

PdmlLacpProtocol::PdmlLacpProtocol()
{
    ostProtoId_ = OstProto::Protocol::kLacpFieldNumber;

    fieldMap_.insert("slow.subtype", OstProto::Lacp::kSubtypeFieldNumber);
    fieldMap_.insert("slow.lacp.version", OstProto::Lacp::kVersionNumberFieldNumber);

    fieldMap_.insert("slow.lacp.actorInfo", OstProto::Lacp::kTlvTypeActorFieldNumber);
    fieldMap_.insert("slow.lacp.actorInfoLen", OstProto::Lacp::kActorInfoLengthFieldNumber);
    fieldMap_.insert("slow.lacp.actorSysPriority", OstProto::Lacp::kActorSystemPriorityFieldNumber);
    fieldMap_.insert("slow.lacp.actorSystem", OstProto::Lacp::kActorSystemFieldNumber);
    fieldMap_.insert("slow.lacp.actorKey", OstProto::Lacp::kActorKeyFieldNumber);
    fieldMap_.insert("slow.lacp.actorPortPriority", OstProto::Lacp::kActorPortPriorityFieldNumber);
    fieldMap_.insert("slow.lacp.actorPort", OstProto::Lacp::kActorPortFieldNumber);
    fieldMap_.insert("slow.lacp.actorState", OstProto::Lacp::kActorStateFieldNumber);

    fieldMap_.insert("slow.lacp.partnerInfo", OstProto::Lacp::kTlvTypePartnerFieldNumber);
    fieldMap_.insert("slow.lacp.partnerInfoLen", OstProto::Lacp::kPartnerInfoLengthFieldNumber);
    fieldMap_.insert("slow.lacp.partnerSysPriority", OstProto::Lacp::kPartnerSystemPriorityFieldNumber);
    fieldMap_.insert("slow.lacp.partnerSystem", OstProto::Lacp::kPartnerSystemFieldNumber);
    fieldMap_.insert("slow.lacp.partnerKey", OstProto::Lacp::kPartnerKeyFieldNumber);
    fieldMap_.insert("slow.lacp.partnerPortPriority", OstProto::Lacp::kPartnerPortPriorityFieldNumber);
    fieldMap_.insert("slow.lacp.partnerPort", OstProto::Lacp::kPartnerPortFieldNumber);
    fieldMap_.insert("slow.lacp.partnerState", OstProto::Lacp::kPartnerStateFieldNumber);

    fieldMap_.insert("slow.lacp.collectorInfo", OstProto::Lacp::kTlvTypeCollectorFieldNumber);
    fieldMap_.insert("slow.lacp.collectorInfoLen", OstProto::Lacp::kCollectorInfoLengthFieldNumber);
    fieldMap_.insert("slow.lacp.collectorMaxDelay", OstProto::Lacp::kCollectorMaxdelayFieldNumber);
    fieldMap_.insert("slow.lacp.termInfo", OstProto::Lacp::kTlvTypeTerminatorFieldNumber);
    fieldMap_.insert("slow.lacp.termLen", OstProto::Lacp::kTerminatorLengthFieldNumber);
}

PdmlLacpProtocol::~PdmlLacpProtocol()
{
}

PdmlProtocol *PdmlLacpProtocol::createInstance()
{
    return new PdmlLacpProtocol();
}

void PdmlLacpProtocol::unknownFieldHandler(QString name, int /*pos*/, int /*size*/, 
                                           const QXmlStreamAttributes &attributes, 
                                           OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;

    OstProto::Lacp *lacp = pbProto->MutableExtension(OstProto::lacp);
    int pos = attributes.value("pos").toString().toInt(&isOk, BASE_DEC);

    if ((name == "slow.lacp.reserved") && (pos == ACTOR_RESERVED_POS))
    {
        lacp->set_actor_reserved((attributes.value("value").toString().toUInt(&isOk, BASE_HEX)));
    }
    if ((name == "slow.lacp.reserved") && (pos == PARTNER_RESERVED_POS))
    {
        lacp->set_partner_reserved((attributes.value("value").toString().toUInt(&isOk, BASE_HEX)));
    }
    if (name == "slow.lacp.coll_reserved")
    {
        lacp->set_collector_reserved(attributes.value("value").toString().toStdString());
    }
    if (name == "slow.lacp.term_reserved")
    {
        lacp->set_terminator_reserved(attributes.value("value").toString().toStdString());
    }
}

