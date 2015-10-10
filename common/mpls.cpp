/*
Copyright (C) 2010, 2014 Srivats P.
Copyright (C) 2015 Ilya Volchanetskiy

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

#include "mpls.h"

MplsProtocol::MplsProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

MplsProtocol::~MplsProtocol()
{
}

AbstractProtocol* MplsProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new MplsProtocol(stream, parent);
}

quint32 MplsProtocol::protocolNumber() const
{
    return OstProto::Protocol::kMplsFieldNumber;
}

void MplsProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::mpls)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void MplsProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::mpls))
        data.MergeFrom(protocol.GetExtension(OstProto::mpls));
}

QString MplsProtocol::name() const
{
    return QString("Multiprotocol Label Switching");
}

QString MplsProtocol::shortName() const
{
    return QString("MPLS");
}

quint32 MplsProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdEth: return 0x8847;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int MplsProtocol::fieldCount() const
{
    return mpls_fieldCount;
}

AbstractProtocol::FieldFlags MplsProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case mpls_label:
        case mpls_exp:
        case mpls_bos:
        case mpls_ttl:
            break;

        case mpls_is_override_bos:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant MplsProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case mpls_label:
        {
            int label = data.label();

            switch(attrib)
            {
                case FieldName:
                    return QString("Label");
                case FieldValue:
                    return label;
                case FieldTextValue:
                    return QString("%1").arg(label);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) label, (uchar*) fv.data());
                    fv.remove(0, 1);
                    return fv;
                }
                case FieldBitSize:
                    return 20;
                default:
                    break;
            }
            break;

        }
        case mpls_exp:
        {
            int exp = data.exp();

            switch(attrib)
            {
                case FieldName:
                    return QString("EXP");
                case FieldValue:
                    return exp;
                case FieldTextValue:
                    return QString("%1").arg(exp);
                case FieldFrameValue:
                    return QByteArray(1, (char) exp);
                case FieldBitSize:
                    return 3;
                default:
                    break;
            }
            break;
        }
        case mpls_bos:
        {
            int bos;
            if(data.is_override_bos())
                bos = data.bos();
            else
                bos = !(next &&
                        next->protocolNumber() ==
                                MplsProtocol::protocolNumber());

            switch(attrib)
            {
                case FieldName:
                    return QString("Bottom of stack");
                case FieldValue:
                    return bos;
                case FieldTextValue:
                    return QString("%1").arg(bos);
                case FieldFrameValue:
                    return QByteArray(1, (char) bos);
                case FieldBitSize:
                    return 1;
                default:
                    break;
            }
            break;
        }
        case mpls_ttl:
        {
            int ttl = data.ttl();

            switch(attrib)
            {
                case FieldName:
                    return QString("TTL");
                case FieldValue:
                    return ttl;
                case FieldTextValue:
                    return QString("%1").arg(ttl);
                case FieldFrameValue:
                    return QByteArray(1, (char) ttl);
                case FieldBitSize:
                    return 8;
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case mpls_is_override_bos:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_bos();
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

bool MplsProtocol::setFieldData(int index, const QVariant &value,
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case mpls_label:
        {
            uint label = value.toUInt(&isOk);
            if (isOk)
                data.set_label(label);
            break;
        }
        case mpls_exp:
        {
            uint exp = value.toUInt(&isOk);
            if (isOk)
                data.set_exp(exp);
            break;
        }
        case mpls_bos:
        {
            uint bos = value.toUInt(&isOk);
            if (isOk)
                data.set_bos(bos);
            break;
        }
        case mpls_ttl:
        {
            uint ttl = value.toUInt(&isOk);
            if (isOk)
                data.set_ttl(ttl);
            break;
        }

        // Meta-fields
        case mpls_is_override_bos:
        {
            bool isOverrideBos = value.toBool();
            data.set_is_override_bos(isOverrideBos);
            isOk = true;
            break;
        }

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

int MplsProtocol::protocolFrameVariableCount() const
{
    return AbstractProtocol::protocolFrameVariableCount();
}
