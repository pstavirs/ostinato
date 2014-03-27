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

#include "snap.h"

quint32 kStdOui = 0x000000;

SnapProtocol::SnapProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

SnapProtocol::~SnapProtocol()
{
}

AbstractProtocol* SnapProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SnapProtocol(stream, parent);
}

quint32 SnapProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSnapFieldNumber;
}

void SnapProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::snap)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SnapProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::snap))
        data.MergeFrom(protocol.GetExtension(OstProto::snap));
}

QString SnapProtocol::name() const
{
    return QString("SubNetwork Access Protocol");
}

QString SnapProtocol::shortName() const
{
    return QString("SNAP");
}

AbstractProtocol::ProtocolIdType SnapProtocol::protocolIdType() const
{
    return ProtocolIdEth;
}

quint32 SnapProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdLlc: return 0xAAAA03;
        default: break;
    }

    return AbstractProtocol::protocolId(type);
}

int SnapProtocol::fieldCount() const
{
    return snap_fieldCount;
}

AbstractProtocol::FieldFlags SnapProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case snap_oui:
        case snap_type:
            break;

        case snap_is_override_oui:
        case snap_is_override_type:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            break;
    }

    return flags;
}

QVariant SnapProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case snap_oui:
            switch(attrib)
            {
                case FieldName:            
                    return QString("OUI");
                case FieldValue:
                {
                    quint32 oui = data.is_override_oui() ? data.oui() : kStdOui;
                    return oui;
                }
                case FieldTextValue:
                {
                    quint32 oui = data.is_override_oui() ? data.oui() : kStdOui;
                    return QString("%1").arg(oui, 6, BASE_HEX, QChar('0'));
                }
                case FieldFrameValue:
                {
                    quint32 oui = data.is_override_oui() ? data.oui() : kStdOui;
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian(oui, (uchar*) fv.data());
                    fv.remove(0, 1);
                    return fv;
                }
                default:
                    break;
            }
            break;
        case snap_type:
        {
            quint16 type;

            switch(attrib)
            {
                case FieldName:            
                    return QString("Type");
                case FieldValue:
                    type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    return type;
                case FieldTextValue:
                    type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    return QString("%1").arg(type, 4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    type = data.is_override_type() ?
                        data.type() : payloadProtocolId(ProtocolIdEth);
                    qToBigEndian(type, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case snap_is_override_oui:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_oui();
                default:
                    break;
            }
            break;
        }
        case snap_is_override_type:
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

bool SnapProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return false;

    switch (index)
    {
        case snap_oui:
        {
            uint oui = value.toUInt(&isOk);
            if (isOk)
                data.set_oui(oui);
            break;
        }
        case snap_type:
        {
            uint type = value.toUInt(&isOk);
            if (isOk)
                data.set_type(type);
            break;
        }
        case snap_is_override_oui:
        {
            bool ovr = value.toBool();
            data.set_is_override_oui(ovr);
            isOk = true;
            break;
        }
        case snap_is_override_type:
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
