/*
Copyright (C) 2016 Srivats P.

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

#include "sign.h"

SignProtocol::SignProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

SignProtocol::~SignProtocol()
{
}

AbstractProtocol* SignProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SignProtocol(stream, parent);
}

quint32 SignProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSignFieldNumber;
}

void SignProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::sign)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SignProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::sign))
        data.MergeFrom(protocol.GetExtension(OstProto::sign));
}

QString SignProtocol::name() const
{
    return QString("Signature");
}

QString SignProtocol::shortName() const
{
    return QString("SIGN");
}

int SignProtocol::fieldCount() const
{
    return sign_fieldCount;
}

AbstractProtocol::FieldFlags SignProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case sign_magic:
        case sign_tlv_guid:
        case sign_tlv_end:
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant SignProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case sign_magic:
        {
            switch(attrib)
            {
                case FieldName:
                    return QString("Magic");
                case FieldValue:
                    return kSignMagic;
                case FieldTextValue:
                    return QString("%1").arg(kSignMagic);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian(kSignMagic, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case sign_tlv_guid:
        {
            quint32 guid = data.stream_guid() & 0xFFFFFF;
            switch(attrib)
            {
                case FieldName:
                    return QString("Stream GUID");
                case FieldValue:
                    return guid;
                case FieldTextValue:
                    return QString("%1").arg(guid);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    fv[0] = (guid >> 16) & 0xff;
                    fv[1] = (guid >>  8) & 0xff;
                    fv[2] = (guid >>  0) & 0xff;
                    fv[3] = kTypeLenGuid;
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case sign_tlv_end:
        {
            switch(attrib)
            {
                case FieldName:
                    return QString("End TLV");
                case FieldValue:
                    return 0;
                case FieldTextValue:
                    return QString("NA");
                case FieldFrameValue:
                    return QByteArray(1, kTypeLenEnd);
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

bool SignProtocol::setFieldData(int index, const QVariant &value,
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case sign_tlv_guid:
        {
            uint guid = value.toUInt(&isOk);
            if (isOk)
                data.set_stream_guid(guid & 0xFFFFFF);
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

quint32 SignProtocol::magic()
{
    return kSignMagic;
}

bool SignProtocol::packetGuid(const uchar *pkt, int pktLen, uint *guid)
{
    const uchar *p = pkt + pktLen - sizeof(kSignMagic);
    quint32 magic = qFromBigEndian<quint32>(p);
    if (magic != kSignMagic)
        return false;

    p--;
    while (*p != kTypeLenEnd) {
        if (*p == kTypeLenGuid) {
            *guid = qFromBigEndian<quint32>(p - 3) >> 8;
            return true;
        }
        p -= 1 + (*p >> 5); // move to next TLV
    }
    return false;
}
