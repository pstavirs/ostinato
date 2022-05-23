/*
Copyright (C) 2021 Srivats P.

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

#include "gre.h"

GreProtocol::GreProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

GreProtocol::~GreProtocol()
{
}

AbstractProtocol* GreProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new GreProtocol(stream, parent);
}

quint32 GreProtocol::protocolNumber() const
{
    return OstProto::Protocol::kGreFieldNumber;
}

void GreProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::gre)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void GreProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::gre))
        data.MergeFrom(protocol.GetExtension(OstProto::gre));
}

QString GreProtocol::name() const
{
    return QString("General Routing Encapsulation Protocol");
}

QString GreProtocol::shortName() const
{
    return QString("GRE");
}

AbstractProtocol::ProtocolIdType GreProtocol::protocolIdType() const
{
    return ProtocolIdEth;
}

quint32 GreProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 47;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int GreProtocol::fieldCount() const
{
    return gre_fieldCount;
}

AbstractProtocol::FieldFlags GreProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    if (index == gre_checksum)
        flags |= CksumField;

    return flags;
}

/*!
TODO: Edit this function to return the data for each field

See AbstractProtocol::fieldData() for more info
*/
QVariant GreProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case gre_flags:
        {
            switch(attrib)
            {
                case FieldName:
                    return QString("Flags");
                case FieldValue:
                    return data.flags();
                case FieldTextValue:
                {
                    QString fstr;
                    fstr.append("Cksum:");
                    fstr.append(data.flags() & GRE_FLAG_CKSUM ? "Y" : "N");
                    fstr.append(" Key:");
                    fstr.append(data.flags() & GRE_FLAG_KEY ? "Y" : "N");
                    fstr.append(" Seq:");
                    fstr.append(data.flags() & GRE_FLAG_SEQ ? "Y" : "N");
                    return fstr;
                }
                case FieldFrameValue:
                    return QByteArray(1, char(data.flags()));
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;
        }
        case gre_rsvd0:
        {
            switch(attrib)
            {
                case FieldName:
                    return QString("Reserved0");
                case FieldValue:
                    return data.rsvd0();
                case FieldTextValue:
                    return QString("%1").arg(data.rsvd0());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(quint16(data.rsvd0()), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return 9;
                default:
                    break;
            }
            break;
        }

        case gre_version:
        {
            switch(attrib)
            {
                case FieldName:
                    return QString("Version");
                case FieldValue:
                    return data.version();
                case FieldFrameValue:
                    return QByteArray(1, char(data.version()));
                case FieldTextValue:
                    return QString("%1").arg(data.version());
                case FieldBitSize:
                    return 3;
                default:
                    break;
            }
            break;
        }
        case gre_protocol:
        {
            quint16 protocol = payloadProtocolId(ProtocolIdEth);

            switch(attrib)
            {
                case FieldName:
                    return QString("Protocol");
                case FieldValue:
                    return protocol;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(protocol, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return  QString("0x%1").arg(
                        protocol, 4, BASE_HEX, QChar('0'));;
                default:
                    break;
            }
            break;
        }
        case gre_checksum:
        {
            if (attrib == FieldName)
                return QString("Checksum");

            if ((data.flags() & GRE_FLAG_CKSUM) == 0)
            {
                if (attrib == FieldTextValue)
                    return QObject::tr("<not-included>");
                else
                    return QVariant();
            }

            if (attrib == FieldBitSize)
                return 16;

            quint32 sum = 0;
            quint16 cksum;

            cksum = protocolFrameCksum(streamIndex, CksumIp);
            sum += (quint16) ~cksum;
            cksum = protocolFramePayloadCksum(streamIndex, CksumIp);
            sum += (quint16) ~cksum;

            while (sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = (~sum) & 0xFFFF;

            switch(attrib)
            {
                case FieldValue:
                    return cksum;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(cksum, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return  QString("0x%1").arg(
                        cksum, 4, BASE_HEX, QChar('0'));;
                default:
                    break;
            }
            break;
        }

        case gre_rsvd1:
        {
            if (attrib == FieldName)
                return QString("Reserved1");

            if ((data.flags() & GRE_FLAG_CKSUM) == 0)
            {
                if (attrib == FieldTextValue)
                    return QObject::tr("<not-included>");
                else
                    return QVariant();
            }

            switch(attrib)
            {
                case FieldValue:
                    return data.rsvd1();
                case FieldTextValue:
                    return QString("%1").arg(data.rsvd1());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.rsvd1(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case gre_key:
        {
            if (attrib == FieldName)
                return QString("Key");

            if ((data.flags() & GRE_FLAG_KEY) == 0)
            {
                if (attrib == FieldTextValue)
                    return QObject::tr("<not-included>");
                else
                    return QVariant();
            }

            switch(attrib)
            {
                case FieldValue:
                    return data.key();
                case FieldTextValue:
                    return QString("0x%1").arg(data.key(), 8, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.key(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case gre_sequence:
        {
            if (attrib == FieldName)
                return QString("Sequence Number");

            if ((data.flags() & GRE_FLAG_SEQ) == 0)
            {
                if (attrib == FieldTextValue)
                    return QObject::tr("<not-included>");
                else
                    return QVariant();
            }

            switch(attrib)
            {
                case FieldValue:
                    return data.sequence_num();
                case FieldTextValue:
                    return QString("%1").arg(data.sequence_num());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.sequence_num(), (uchar*) fv.data());
                    return fv;
                }
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

/*!
TODO: Edit this function to set the data for each field

See AbstractProtocol::setFieldData() for more info
*/
bool GreProtocol::setFieldData(int index, const QVariant &value,
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case gre_flags:
        {
            uint flags = value.toUInt(&isOk);
            if (isOk)
                data.set_flags(flags);
            break;
        }
        case gre_rsvd0:
        {
            uint rsvd0 = value.toUInt(&isOk);
            if (isOk)
                data.set_rsvd0(rsvd0);
            break;
        }
        case gre_version:
        {
            uint ver = value.toUInt(&isOk);
            if (isOk)
                data.set_version(ver);
            break;
        }
        case gre_protocol:
        {
            uint proto = value.toUInt(&isOk);
            if (isOk)
                data.set_protocol_type(proto);
            break;
        }
        case gre_checksum:
        {
            uint csum = value.toUInt(&isOk);
            if (isOk)
                data.set_checksum(csum);
            break;
        }
        case gre_rsvd1:
        {
            uint rsvd1 = value.toUInt(&isOk);
            if (isOk)
                data.set_rsvd1(rsvd1);
            break;
        }
        case gre_key:
        {
            uint key = value.toUInt(&isOk);
            if (isOk)
                data.set_key(key);
            break;
        }
        case gre_sequence:
        {
            uint seq = value.toUInt(&isOk);
            if (isOk)
                data.set_sequence_num(seq);
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

/*!
  TODO: Return the protocol frame size in bytes\n

  If your protocol has a fixed size - you don't need to reimplement this; the
  base class implementation is good enough
*/
int GreProtocol::protocolFrameSize(int /*streamIndex*/) const
{
    int size = 4; // mandatory fields - flags, rsvd0, version, protocol

    if (data.flags() & GRE_FLAG_CKSUM)
        size += 4;
    if (data.flags() & GRE_FLAG_KEY)
        size += 4;
    if (data.flags() & GRE_FLAG_SEQ)
        size += 4;

    return size;
}
