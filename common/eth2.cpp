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

#include "eth2.h"

Eth2Protocol::Eth2Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

Eth2Protocol::~Eth2Protocol()
{
}

AbstractProtocol* Eth2Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Eth2Protocol(stream, parent);
}

quint32 Eth2Protocol::protocolNumber() const
{
    return OstProto::Protocol::kEth2FieldNumber;
}

void Eth2Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::eth2)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Eth2Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::eth2))
        data.MergeFrom(protocol.GetExtension(OstProto::eth2));
}

QString Eth2Protocol::name() const
{
    return QString("Ethernet II");
}

QString Eth2Protocol::shortName() const
{
    return QString("Eth II");
}

AbstractProtocol::ProtocolIdType Eth2Protocol::protocolIdType() const
{
    return ProtocolIdEth;
}

int    Eth2Protocol::fieldCount() const
{
    return eth2_fieldCount;
}

AbstractProtocol::FieldFlags Eth2Protocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case eth2_type:
            break;

        case eth2_is_override_type:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            break;
    }

    return flags;
}

QVariant Eth2Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case eth2_type:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Type");
                case FieldValue:
                {
                    quint16 type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    return type;
                }
                case FieldTextValue:
                {
                    quint16 type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    return QString("0x%1").arg(type, 4, BASE_HEX, QChar('0'));
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    quint16 type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    fv.resize(2);
                    qToBigEndian((quint16) type, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case eth2_is_override_type:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_type();
                default:
                    break;
            }
            break;
        }

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool Eth2Protocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return false;

    switch (index)
    {
        case eth2_type:
        {
            uint type = value.toUInt(&isOk);
            if (isOk)
                data.set_type(type);
            break;
        }
        case eth2_is_override_type:
        {
            bool ovr = value.toBool();
            data.set_is_override_type(ovr);
            isOk = true;
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }
    return isOk;
}
