/*
Copyright (C) 2010 Srivats P.

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

#ifndef _ICMP_HELPER_H
#define _ICMP_HELPER_H 

#include "icmp.pb.h"

#include <QSet>

enum IcmpType
{
    kIcmpEchoReply                = 0,
    kIcmpDestinationUnreachable   = 3,
    kIcmpSourceQuench             = 4,
    kIcmpRedirect                 = 5,
    kIcmpEchoRequest              = 8,
    kIcmpTimeExceeded             = 11,
    kIcmpParameterProblem         = 12,
    kIcmpTimestampRequest         = 13,
    kIcmpTimestampReply           = 14,
    kIcmpInformationRequest       = 15,
    kIcmpInformationReply         = 16,
    kIcmpAddressMaskRequest       = 17,
    kIcmpAddressMaskReply         = 18
};

enum Icmp6Type
{
    kIcmp6DestinationUnreachable   = 1,
    kIcmp6PacketTooBig             = 2,
    kIcmp6TimeExceeded             = 3,
    kIcmp6ParameterProblem         = 4,
    kIcmp6EchoRequest              = 128,
    kIcmp6EchoReply                = 129,
    kIcmp6RouterSolicitation       = 133,
    kIcmp6RouterAdvertisement      = 134,
    kIcmp6NeighbourSolicitation    = 135,
    kIcmp6NeighbourAdvertisement   = 136,
    kIcmp6Redirect                 = 137,
    kIcmp6InformationQuery         = 139,
    kIcmp6InformationResponse      = 140
};

static QSet<int> icmpIdSeqSet = QSet<int>()
    << kIcmpEchoRequest
    << kIcmpEchoReply
    << kIcmpInformationRequest
    << kIcmpInformationReply;

static QSet<int> icmp6IdSeqSet = QSet<int>()
    << kIcmp6EchoRequest
    << kIcmp6EchoReply;

bool inline isIdSeqType(OstProto::Icmp::Version ver, int type)
{
    //qDebug("%s: ver = %d, type = %d", __FUNCTION__, ver, type);
    switch(ver)
    {
    case OstProto::Icmp::kIcmp4:
        return icmpIdSeqSet.contains(type);
    case OstProto::Icmp::kIcmp6:
        return icmp6IdSeqSet.contains(type);
    default:
        break;
    }

    Q_ASSERT(false); // unreachable
    return false;
}

#endif
