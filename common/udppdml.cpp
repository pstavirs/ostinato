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

#include "udppdml.h"

#include "udp.pb.h"

PdmlUdpProtocol::PdmlUdpProtocol()
{
    ostProtoId_ = OstProto::Protocol::kUdpFieldNumber;

    fieldMap_.insert("udp.srcport", OstProto::Udp::kSrcPortFieldNumber);
    fieldMap_.insert("udp.dstport", OstProto::Udp::kDstPortFieldNumber);
    fieldMap_.insert("udp.length", OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum_coverage", 
                     OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum", OstProto::Udp::kCksumFieldNumber);
}

PdmlProtocol* PdmlUdpProtocol::createInstance()
{
    return new PdmlUdpProtocol();
}

void PdmlUdpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream* /*stream*/)
{
    OstProto::Udp *udp = pbProto->MutableExtension(OstProto::udp);

    qDebug("Udp: post\n");

    udp->set_is_override_src_port(true);
    udp->set_is_override_dst_port(true);
    udp->set_is_override_totlen(true);
    udp->set_is_override_cksum(true);
}

