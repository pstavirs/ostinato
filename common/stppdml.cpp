/*
Copyright (C) 2014 PLVision.

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

This module is developed by PLVision  <developers@plvision.eu>
*/

#include "stppdml.h"

#include "stp.pb.h"

#define ROOT_IDENTIFIER_POS 22
#define BRIDGE_IDENTIFIER_POS 34
#define BASE_DEC 10
#define BASE_HEX 16

PdmlStpProtocol::PdmlStpProtocol()
{
    ostProtoId_ = OstProto::Protocol::kStpFieldNumber;

    fieldMap_.insert("stp.protocol",
                     OstProto::Stp::kProtocolIdFieldNumber);
    fieldMap_.insert("stp.version",
                     OstProto::Stp::kProtocolVersionIdFieldNumber);
    fieldMap_.insert("stp.type", OstProto::Stp::kBpduTypeFieldNumber);
    fieldMap_.insert("stp.flags", OstProto::Stp::kFlagsFieldNumber);
    fieldMap_.insert("stp.root.cost", OstProto::Stp::kRootPathCostFieldNumber);
    fieldMap_.insert("stp.port", OstProto::Stp::kPortIdFieldNumber);
    fieldMap_.insert("stp.msg_age", OstProto::Stp::kMessageAgeFieldNumber);
    fieldMap_.insert("stp.max_age", OstProto::Stp::kMaxAgeFieldNumber);
    fieldMap_.insert("stp.hello", OstProto::Stp::kHelloTimeFieldNumber);
    fieldMap_.insert("stp.forward", OstProto::Stp::kForwardDelayFieldNumber);
}

PdmlStpProtocol::~PdmlStpProtocol()
{
}

PdmlProtocol* PdmlStpProtocol::createInstance()
{
    return new PdmlStpProtocol();
}

void PdmlStpProtocol::unknownFieldHandler(
        QString name, int pos, int /*size*/,
        const QXmlStreamAttributes &attributes, OstProto::Protocol *pbProto,
        OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Stp *stp = pbProto->MutableExtension(OstProto::stp);

    if ((name == "") && (pos == ROOT_IDENTIFIER_POS))
    {
        stp->set_root_id(attributes.value("value").toString().
                         toULongLong(&isOk, BASE_HEX));
    }
    if ((name == "") && (pos == BRIDGE_IDENTIFIER_POS))
    {
        stp->set_bridge_id(attributes.value("value").toString().
                           toULongLong(&isOk, BASE_HEX));
    }
}
