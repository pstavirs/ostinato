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

#include "llc.h"

LlcProtocol::LlcProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

LlcProtocol::~LlcProtocol()
{
}

AbstractProtocol* LlcProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new LlcProtocol(stream, parent);
}

quint32 LlcProtocol::protocolNumber() const
{
    return OstProto::Protocol::kLlcFieldNumber;
}

void LlcProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::llc)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void LlcProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::llc))
        data.MergeFrom(protocol.GetExtension(OstProto::llc));
}

QString LlcProtocol::name() const
{
    return QString("802.3 Logical Link Control");
}

QString LlcProtocol::shortName() const
{
    return QString("LLC");
}

AbstractProtocol::ProtocolIdType LlcProtocol::protocolIdType() const
{
    return ProtocolIdLlc;
}

int    LlcProtocol::fieldCount() const
{
    return llc_fieldCount;
}

AbstractProtocol::FieldFlags LlcProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case llc_dsap:
        case llc_ssap:
        case llc_ctl:
            break;

        case llc_is_override_dsap:
        case llc_is_override_ssap:
        case llc_is_override_ctl:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            break;
    }

    return flags;
}

QVariant LlcProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    quint32 id;
    quint8 dsap, ssap, ctl;

    id = payloadProtocolId(ProtocolIdLlc);
    dsap = data.is_override_dsap() ? data.dsap() : (id >> 16) & 0xFF;
    ssap = data.is_override_ssap() ? data.ssap() : (id >> 8) & 0xFF;
    ctl  = data.is_override_ctl() ? data.ctl() : (id >> 0) & 0xFF;

    switch (index)
    {
        case llc_dsap:
            switch(attrib)
            {
                case FieldName:            
                    return QString("DSAP");
                case FieldValue:
                    return dsap;
                case FieldTextValue:
                    return QString("%1").arg(dsap, 2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)(dsap));
                default:
                    break;
            }
            break;
        case llc_ssap:
            switch(attrib)
            {
                case FieldName:            
                    return QString("SSAP");
                case FieldValue:
                    return ssap;
                case FieldTextValue:
                    return QString("%1").arg(ssap, 2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)(ssap));
                default:
                    break;
            }
            break;
        case llc_ctl:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Control");
                case FieldValue:
                    return ctl;
                case FieldTextValue:
                    return QString("%1").arg(ctl, 2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)(ctl));
                default:
                    break;
            }
            break;


        // Meta fields
        case llc_is_override_dsap:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_dsap();
                default:
                    break;
            }
            break;
        }
        case llc_is_override_ssap:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_ssap();
                default:
                    break;
            }
            break;
        }
        case llc_is_override_ctl:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_ctl();
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

bool LlcProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return false;

    switch (index)
    {
        case llc_dsap:
        {
            uint dsap = value.toUInt(&isOk) & 0xFF;
            if (isOk)
                data.set_dsap(dsap);
            break;
        }
        case llc_ssap:
        {
            uint ssap = value.toUInt(&isOk) & 0xFF;
            if (isOk)
                data.set_ssap(ssap);
            break;
        }
        case llc_ctl:
        {
            uint ctl = value.toUInt(&isOk) & 0xFF;
            if (isOk)
                data.set_ctl(ctl);
            break;
        }
        case llc_is_override_dsap:
        {
            bool ovr = value.toBool();
            data.set_is_override_dsap(ovr);
            isOk = true;
            break;
        }
        case llc_is_override_ssap:
        {
            bool ovr = value.toBool();
            data.set_is_override_ssap(ovr);
            isOk = true;
            break;
        }
        case llc_is_override_ctl:
        {
            bool ovr = value.toBool();
            data.set_is_override_ctl(ovr);
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
