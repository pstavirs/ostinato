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

#include "icmp6pdml.h"

#include "icmp.pb.h"
#include "sample.pb.h"

PdmlIcmp6Protocol::PdmlIcmp6Protocol()
{
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;

    proto_ = NULL;
}

PdmlProtocol* PdmlIcmp6Protocol::createInstance()
{
    return new PdmlIcmp6Protocol();
}

void PdmlIcmp6Protocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, 
        int expectedPos, OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    proto_ = NULL;
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;
    icmp_.preProtocolHandler(name, attributes, expectedPos, pbProto, stream);
    mld_.preProtocolHandler(name, attributes, expectedPos, pbProto, stream);
}

void PdmlIcmp6Protocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    if (proto_)
        proto_->postProtocolHandler(pbProto, stream);
    else
        stream->mutable_protocol()->RemoveLast();

    proto_ = NULL;
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;
}

void PdmlIcmp6Protocol::unknownFieldHandler(QString name, 
        int pos, int size, const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (proto_)
    {
        proto_->unknownFieldHandler(name, pos, size, attributes, pbProto, 
            stream);
    }
    else if (name == "icmpv6.type")
    {
        bool isOk;
        uint type =  attributes.value("value").toString().toUInt(
                &isOk, kBaseHex);

        if (((type >= 130) && (type <= 132)) || (type == 143))
        {
            // MLD
            proto_ = &mld_;
            fieldMap_ = mld_.fieldMap_;
            ostProtoId_ = OstProto::Protocol::kMldFieldNumber;
        }
        else
        {
            // ICMP
            proto_ = &icmp_;
            fieldMap_ = icmp_.fieldMap_;
            ostProtoId_ = OstProto::Protocol::kIcmpFieldNumber;
        }

        pbProto->mutable_protocol_id()->set_id(ostProtoId_);
        pbProto->MutableExtension(OstProto::sample)->Clear();

        fieldHandler(name, attributes, pbProto, stream);
    }
    else
    {
        qDebug("unexpected field %s", name.toAscii().constData());
    }
}

