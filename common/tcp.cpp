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

#include "tcp.h"


TcpProtocol::TcpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

TcpProtocol::~TcpProtocol()
{
}

AbstractProtocol* TcpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new TcpProtocol(stream, parent);
}

quint32 TcpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kTcpFieldNumber;
}

void TcpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::tcp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void TcpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::tcp))
        data.MergeFrom(protocol.GetExtension(OstProto::tcp));
}

QString TcpProtocol::name() const
{
    return QString("Transmission Control Protocol");
}

QString TcpProtocol::shortName() const
{
    return QString("TCP");
}

AbstractProtocol::ProtocolIdType TcpProtocol::protocolIdType() const
{
    return ProtocolIdTcpUdp;
}

quint32 TcpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x06;
        default: break;
    }

    return AbstractProtocol::protocolId(type);
}

int TcpProtocol::fieldCount() const
{
    return tcp_fieldCount;
}

AbstractProtocol::FieldFlags TcpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case tcp_src_port:
        case tcp_dst_port:
        case tcp_seq_num:
        case tcp_ack_num:
        case tcp_hdrlen:
        case tcp_rsvd:
        case tcp_flags:
        case tcp_window:
            break;

        case tcp_cksum:
            flags |= CksumField;
            break;

        case tcp_urg_ptr:
            break;

        case tcp_is_override_src_port:
        case tcp_is_override_dst_port:
        case tcp_is_override_hdrlen:
        case tcp_is_override_cksum:
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

QVariant TcpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case tcp_src_port:
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
        case tcp_dst_port:
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
        case tcp_seq_num:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sequence Number");
                case FieldValue:
                    return data.seq_num();
                case FieldTextValue:
                    return QString("%1").arg(data.seq_num());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.seq_num(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        case tcp_ack_num:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Acknowledgement Number");
                case FieldValue:
                    return data.ack_num();
                case FieldTextValue:
                    return QString("%1").arg(data.ack_num());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.ack_num(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        case tcp_hdrlen:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Header Length");
                case FieldValue:
                    if (data.is_override_hdrlen())
                        return ((data.hdrlen_rsvd() >> 4) & 0x0F);
                    else
                        return 5;
                case FieldTextValue:
                    if (data.is_override_hdrlen())
                        return QString("%1 bytes").arg(
                            4 * ((data.hdrlen_rsvd() >> 4) & 0x0F));
                    else
                        return QString("20 bytes");
                case FieldFrameValue:
                    if (data.is_override_hdrlen())
                        return QByteArray(1,
                            (char)((data.hdrlen_rsvd() >> 4) & 0x0F));
                    else
                        return QByteArray(1, (char) 0x05);
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;

        case tcp_rsvd:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Reserved");
                case FieldValue:
                    return (data.hdrlen_rsvd() & 0x0F);
                case FieldTextValue:
                    return QString("%1").arg(data.hdrlen_rsvd() & 0x0F);
                case FieldFrameValue:
                    return QByteArray(1, (char)(data.hdrlen_rsvd() & 0x0F));
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;

        case tcp_flags:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Flags");
                case FieldValue:
                    return (data.flags());
                case FieldTextValue:
                {
                    QString s;
                    s.append("URG: ");
                    s.append(data.flags() & TCP_FLAG_URG ? "1" : "0");
                    s.append(" ACK: ");
                    s.append(data.flags() & TCP_FLAG_ACK ? "1" : "0");
                    s.append(" PSH: ");
                    s.append(data.flags() & TCP_FLAG_PSH ? "1" : "0");
                    s.append(" RST: ");
                    s.append(data.flags() & TCP_FLAG_RST ? "1" : "0");
                    s.append(" SYN: ");
                    s.append(data.flags() & TCP_FLAG_SYN ? "1" : "0");
                    s.append(" FIN: ");
                    s.append(data.flags() & TCP_FLAG_FIN ? "1" : "0");
                    return s;
                }
                case FieldFrameValue:
                    return QByteArray(1, (char)(data.flags() & 0x3F));
                default:
                    break;
            }
            break;

        case tcp_window:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Window Size");
                case FieldValue:
                    return data.window();
                case FieldTextValue:
                    return QString("%1").arg(data.window());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.window(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        case tcp_cksum:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Checksum");
                case FieldValue:
                {
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else 
                        cksum = protocolFrameCksum(streamIndex, CksumTcpUdp);

                    return cksum;
                }
                case FieldTextValue:
                {
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else 
                        cksum = protocolFrameCksum(streamIndex, CksumTcpUdp);

                    return QString("0x%1").arg(cksum, 4, BASE_HEX, QChar('0'));
                }
                case FieldFrameValue:
                {
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else 
                        cksum = protocolFrameCksum(streamIndex, CksumTcpUdp);

                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(cksum, (uchar*) fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;

        case tcp_urg_ptr:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Urgent Pointer");
                case FieldValue:
                    return data.urg_ptr();
                case FieldTextValue:
                    return QString("%1").arg(data.urg_ptr());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.urg_ptr(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        // Meta fields
        case tcp_is_override_src_port:
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
        case tcp_is_override_dst_port:
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
        case tcp_is_override_hdrlen:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_hdrlen();
                default:
                    break;
            }
            break;
        }
        case tcp_is_override_cksum:
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

bool TcpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case tcp_src_port:
        {
            uint srcPort = value.toUInt(&isOk);
            if (isOk)
                data.set_src_port(srcPort);
            break;
        }
        case tcp_dst_port:
        {
            uint dstPort = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_port(dstPort);
            break;
        }
        case tcp_seq_num:
        {
            uint seqNum = value.toUInt(&isOk);
            if (isOk)
                data.set_seq_num(seqNum);
            break;
        }
        case tcp_ack_num:
        {
            uint ackNum = value.toUInt(&isOk);
            if (isOk)
                data.set_ack_num(ackNum);
            break;
        }
        case tcp_hdrlen:
        {
            uint hdrLen = value.toUInt(&isOk);
            if (isOk)
                data.set_hdrlen_rsvd(
                    (data.hdrlen_rsvd() & 0x0F) | (hdrLen << 4));
            break;
        }
        case tcp_rsvd:
        {
            uint rsvd = value.toUInt(&isOk);
            if (isOk)
                data.set_hdrlen_rsvd(
                    (data.hdrlen_rsvd() & 0xF0) | (rsvd & 0x0F));
            break;
        }
        case tcp_flags:
        {
            uint flags = value.toUInt(&isOk);
            if (isOk)
                data.set_flags(flags);
            break;
        }
        case tcp_window:
        {
            uint window = value.toUInt(&isOk);
            if (isOk)
                data.set_window(window);
            break;
        }
        case tcp_cksum:
        {
            uint cksum = value.toUInt(&isOk);
            if (isOk)
                data.set_cksum(cksum);
            break;
        }
        case tcp_urg_ptr:
        {
            uint urgPtr = value.toUInt(&isOk);
            if (isOk)
                data.set_urg_ptr(urgPtr);
            break;
        }
        case tcp_is_override_src_port:
        {
            data.set_is_override_src_port(value.toBool());
            isOk = true;
            break;
        }
        case tcp_is_override_dst_port:
        {
            data.set_is_override_dst_port(value.toBool());
            isOk = true;
            break;
        }
        case tcp_is_override_hdrlen:
        {
            data.set_is_override_hdrlen(value.toBool());
            isOk = true;
            break;
        }
        case tcp_is_override_cksum:
        {
            data.set_is_override_cksum(value.toBool());
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

bool TcpProtocol::isProtocolFrameValueVariable() const
{
    if (data.is_override_cksum())
        return false;
    else
        return isProtocolFramePayloadValueVariable();
}

int TcpProtocol::protocolFrameVariableCount() const
{
    if (data.is_override_cksum())
        return 1;

    return protocolFramePayloadVariableCount();
}

