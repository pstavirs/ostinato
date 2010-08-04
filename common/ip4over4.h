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

#ifndef _IP_4_OVER_4_H
#define _IP_4_OVER_4_H

#include "ip4over4.pb.h"

#include "comboprotocol.h"
#include "ip4.h"

typedef ComboProtocol<OstProto::Protocol::kIp4over4FieldNumber, 
    Ip4Protocol, Ip4Protocol> Ip4over4Combo;

class Ip4over4Protocol : public Ip4over4Combo 
{
public:
    Ip4over4Protocol(StreamBase *stream, AbstractProtocol *parent = 0)
        : Ip4over4Combo(stream, parent) 
    {
    }

    static Ip4over4Protocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent)
    {
        return new Ip4over4Protocol(stream, parent);
    }

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const
    {
        OstProto::Protocol tempProto;

        protoA->protoDataCopyInto(tempProto);
        protocol.MutableExtension(OstProto::ip4over4)
            ->MutableExtension(OstProto::ip4_outer)
            ->CopyFrom(tempProto.GetExtension(OstProto::ip4));

        tempProto.Clear();

        protoB->protoDataCopyInto(tempProto);
        protocol.MutableExtension(OstProto::ip4over4)
            ->MutableExtension(OstProto::ip4_inner)
            ->CopyFrom(tempProto.GetExtension(OstProto::ip4));

        protocol.mutable_protocol_id()->set_id(protocolNumber());
    }

    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol)
    {
        if (protocol.protocol_id().id() == protocolNumber()
                && protocol.HasExtension(OstProto::ip4over4))
        {
            OstProto::Protocol    tempProto;

            // NOTE: To use protoX->protoDataCopyFrom() we need to arrange
            // so that it sees its own protocolNumber() and its own extension
            // in 'protocol'
            tempProto.mutable_protocol_id()->set_id(protoA->protocolNumber());
            tempProto.MutableExtension(OstProto::ip4)->CopyFrom(
                    protocol.GetExtension(OstProto::ip4over4).GetExtension(
                        OstProto::ip4_outer));
            protoA->protoDataCopyFrom(tempProto);

            tempProto.Clear();

            tempProto.mutable_protocol_id()->set_id(protoB->protocolNumber());
            tempProto.MutableExtension(OstProto::ip4)->CopyFrom(
                    protocol.GetExtension(OstProto::ip4over4).GetExtension(
                        OstProto::ip4_inner));
            protoB->protoDataCopyFrom(tempProto);
        }
    }
};

#endif
