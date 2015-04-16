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

#include "udp.h"

UdpProtocol::UdpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

UdpProtocol::~UdpProtocol()
{
}

AbstractProtocol* UdpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new UdpProtocol(stream, parent);
}

quint32 UdpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kUdpFieldNumber;
}

void UdpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::udp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void UdpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::udp))
        data.MergeFrom(protocol.GetExtension(OstProto::udp));
}

QString UdpProtocol::name() const
{
    return QString("User Datagram Protocol");
}

QString UdpProtocol::shortName() const
{
    return QString("UDP");
}

AbstractProtocol::ProtocolIdType UdpProtocol::protocolIdType() const
{
    return ProtocolIdTcpUdp;
}

quint32 UdpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x11;
        default: break;
    }

    return AbstractProtocol::protocolId(type);
}

int UdpProtocol::fieldCount() const
{
    return udp_fieldCount;
}

AbstractProtocol::FieldFlags UdpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case udp_srcPort:
        case udp_dstPort:
        case udp_totLen:
            break;

        case udp_cksum:
            flags |= CksumField;
            break;

        case udp_isOverrideSrcPort:
        case udp_isOverrideDstPort:
        case udp_isOverrideTotLen:
        case udp_isOverrideCksum:
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

QVariant UdpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case udp_srcPort:
        {
            quint16 srcPort;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_src_port())
                        srcPort = data.src_port();
                    else
                        srcPort = payloadProtocolId(ProtocolIdTcpUdp);
                    break;
                default:
                    srcPort = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Source Port");
                case FieldValue:
                    return srcPort;
                case FieldTextValue:
                    return QString("%1").arg(srcPort);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(srcPort, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case udp_dstPort:
        {
            quint16 dstPort;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_dst_port())
                        dstPort = data.dst_port();
                    else
                        dstPort = payloadProtocolId(ProtocolIdTcpUdp);
                    break;
                default:
                    dstPort = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination Port");
                case FieldValue:
                    return dstPort;
                case FieldTextValue:
                    return QString("%1").arg(dstPort);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(dstPort, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case udp_totLen:
        {

            switch(attrib)
            {
                case FieldName:            
                    return QString("Datagram Length");
                case FieldValue:
                {
                    int totlen;

                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    return totlen;
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int totlen;
                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    fv.resize(2);
                    qToBigEndian((quint16) totlen, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                {
                    int totlen;
                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    return QString("%1").arg(totlen);
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case udp_cksum:
        {
            quint16 cksum;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                {
                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumTcpUdp);
                    qDebug("UDP cksum = %hu", cksum);
                    break;
                }
                default:
                    cksum = 0;
                    break;
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Checksum");
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
                    return  QString("0x%1").
                        arg(cksum, 4, BASE_HEX, QChar('0'));;
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case udp_isOverrideSrcPort:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_src_port();
                default:
                    break;
            }
            break;
        }
        case udp_isOverrideDstPort:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_dst_port();
                default:
                    break;
            }
            break;
        }
        case udp_isOverrideTotLen:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_totlen();
                default:
                    break;
            }
            break;
        }
        case udp_isOverrideCksum:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_cksum();
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

bool UdpProtocol::setFieldData(int index, const QVariant& value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case udp_isOverrideSrcPort:
        {
            data.set_is_override_src_port(value.toBool());
            isOk = true;
            break;
        }
        case udp_isOverrideDstPort:
        {
            data.set_is_override_dst_port(value.toBool());
            isOk = true;
            break;
        }
        case udp_isOverrideTotLen:
        {
            data.set_is_override_totlen(value.toBool());
            isOk = true;
            break;
        }
        case udp_isOverrideCksum:
        {
            data.set_is_override_cksum(value.toBool());
            isOk = true;
            break;
        }
        case udp_srcPort:
        {
            uint srcPort = value.toUInt(&isOk);
            if (isOk)
                data.set_src_port(srcPort);
            break;
        }
        case udp_dstPort:
        {
            uint dstPort = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_port(dstPort);
            break;
        }
        case udp_totLen:
        {
            uint totLen = value.toUInt(&isOk);
            if (isOk)
                data.set_totlen(totLen);
            break;
        }
        case udp_cksum:
        {
            uint cksum = value.toUInt(&isOk);
            if (isOk)
                data.set_cksum(cksum);
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

bool UdpProtocol::isProtocolFrameValueVariable() const
{
    if (data.is_override_totlen() && data.is_override_cksum())
        return false;
    else
        return isProtocolFramePayloadValueVariable();
}

int UdpProtocol::protocolFrameVariableCount() const
{
    if (data.is_override_totlen() && data.is_override_cksum())
        return 1;

    return protocolFramePayloadVariableCount();
}
