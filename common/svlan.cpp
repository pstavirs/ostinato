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

#include "svlan.h"
#include "svlan.pb.h"

SVlanProtocol::SVlanProtocol(StreamBase *stream, AbstractProtocol *parent)
    : VlanProtocol(stream, parent)
{
    data.set_tpid(0x88a8);
    data.set_is_override_tpid(true);
}

SVlanProtocol::~SVlanProtocol()
{
}

AbstractProtocol* SVlanProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SVlanProtocol(stream, parent);
}

quint32 SVlanProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSvlanFieldNumber;
}

void SVlanProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::svlan)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SVlanProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::svlan))
        data.MergeFrom(protocol.GetExtension(OstProto::svlan));
}

QString SVlanProtocol::name() const
{
    return QString("SVlan");
}

QString SVlanProtocol::shortName() const
{
    return QString("SVlan");
}
