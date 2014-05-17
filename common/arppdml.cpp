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

#include "arppdml.h"

#include "arp.pb.h"

PdmlArpProtocol::PdmlArpProtocol()
{
    ostProtoId_ = OstProto::Protocol::kArpFieldNumber;

    fieldMap_.insert("arp.opcode", OstProto::Arp::kOpCodeFieldNumber);
    fieldMap_.insert("arp.src.hw_mac", OstProto::Arp::kSenderHwAddrFieldNumber);
    fieldMap_.insert("arp.src.proto_ipv4", 
            OstProto::Arp::kSenderProtoAddrFieldNumber);
    fieldMap_.insert("arp.dst.hw_mac", OstProto::Arp::kTargetHwAddrFieldNumber);
    fieldMap_.insert("arp.dst.proto_ipv4", 
            OstProto::Arp::kTargetProtoAddrFieldNumber);
}

PdmlProtocol* PdmlArpProtocol::createInstance()
{
    return new PdmlArpProtocol();
}

