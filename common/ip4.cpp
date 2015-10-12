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

#include "ip4.h"

#include <QHostAddress>

Ip4Protocol::Ip4Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

Ip4Protocol::~Ip4Protocol()
{
}

AbstractProtocol* Ip4Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Ip4Protocol(stream, parent);
}

quint32 Ip4Protocol::protocolNumber() const
{
    return OstProto::Protocol::kIp4FieldNumber;
}

void Ip4Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::ip4)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Ip4Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::ip4))
        data.MergeFrom(protocol.GetExtension(OstProto::ip4));
}

QString Ip4Protocol::name() const
{
    return QString("Internet Protocol ver 4");
}

QString Ip4Protocol::shortName() const
{
    return QString("IPv4");
}

AbstractProtocol::ProtocolIdType Ip4Protocol::protocolIdType() const
{
    return ProtocolIdIp;
}

quint32 Ip4Protocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdLlc: return 0x060603;
        case ProtocolIdEth: return 0x0800;
        case ProtocolIdIp: return 0x04;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int    Ip4Protocol::fieldCount() const
{
    return ip4_fieldCount;
}

AbstractProtocol::FieldFlags Ip4Protocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case ip4_ver:
        case ip4_hdrLen:
        case ip4_tos:
        case ip4_totLen:
        case ip4_id:
        case ip4_flags:
        case ip4_fragOfs:
        case ip4_ttl:
        case ip4_proto:
            break;

        case ip4_cksum:
            flags |= CksumField;
            break;

        case ip4_srcAddr:
        case ip4_dstAddr:
            break;

        case ip4_isOverrideVer:
        case ip4_isOverrideHdrLen:
        case ip4_isOverrideTotLen:
        case ip4_isOverrideProto:
        case ip4_isOverrideCksum:
        case ip4_srcAddrMode:
        case ip4_srcAddrCount:
        case ip4_srcAddrMask:
        case ip4_dstAddrMode:
        case ip4_dstAddrCount:
        case ip4_dstAddrMask:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            break;
    }

    return flags;
}

QVariant Ip4Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case ip4_ver:
        {
            int ver;

            ver = data.is_override_ver() ? (data.ver_hdrlen() >> 4) & 0x0F : 4;

            switch(attrib)
            {
                case FieldName:            
                    return QString("Version");
                case FieldValue:
                    return ver;
                case FieldTextValue:
                    return QString("%1").arg(ver, 1, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char) ver);
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;
        }
        case ip4_hdrLen:
        {
            int hdrlen;

            hdrlen = data.is_override_hdrlen() ? data.ver_hdrlen() & 0x0F : 5;

            switch(attrib)
            {
                case FieldName:            
                    return QString("Header Length");
                case FieldValue:
                    return hdrlen;
                case FieldTextValue:
                    return QString("%1").arg(hdrlen, 1, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char) hdrlen);
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;
        }
        case ip4_tos:
            switch(attrib)
            {
                case FieldName:            
                    return QString("TOS/DSCP");
                case FieldValue:
                    return data.tos();
                case FieldFrameValue:
                    return QByteArray(1, (char) data.tos());
                case FieldTextValue:
                    return QString("0x%1").
                        arg(data.tos(), 2, BASE_HEX, QChar('0'));;
                default:
                    break;
            }
            break;
        case ip4_totLen:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Total Length");
                case FieldValue:
                {
                    int totlen;
                    totlen = data.is_override_totlen() ? data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 20);
                    return totlen;
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int totlen;
                    totlen = data.is_override_totlen() ? data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 20);
                    fv.resize(2);
                    qToBigEndian((quint16) totlen, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                {
                    int totlen;
                    totlen = data.is_override_totlen() ? data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 20);
                    return QString("%1").arg(totlen);
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case ip4_id:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Identification");
                case FieldValue:
                    return data.id();
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.id(), (uchar*)fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("0x%1").
                        arg(data.id(), 2, BASE_HEX, QChar('0'));;
                default:
                    break;
            }
            break;
        case ip4_flags:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Flags");
                case FieldValue:
                    return data.flags();
                case FieldFrameValue:
                    return QByteArray(1, (char) data.flags());
                case FieldTextValue:
                {
                    QString s;
                    s.append("Unused:");
                    s.append(data.flags() & IP_FLAG_UNUSED ? "1" : "0");
                    s.append("  Don't Fragment:");
                    s.append(data.flags() & IP_FLAG_DF ? "1" : "0");
                    s.append("  More Fragments:");
                    s.append(data.flags() & IP_FLAG_MF ? "1" : "0");
                    return s;
                }
                case FieldBitSize:
                    return 3;
                default:
                    break;
            }
            break;
        case ip4_fragOfs:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Fragment Offset");
                case FieldValue:
                    return data.frag_ofs();
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) (data.frag_ofs()),
                        (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("%1").arg(data.frag_ofs()*8);
                case FieldBitSize:
                    return 13;
                default:
                    break;
            }
            break;
        case ip4_ttl:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Time to Live");
                case FieldValue:
                    return data.ttl();
                case FieldFrameValue:
                    return QByteArray(1, (char)data.ttl());
                case FieldTextValue:
                    return QString("%1").arg(data.ttl());
                default:
                    break;
            }
            break;
        case ip4_proto:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Protocol");
                case FieldValue:
                {
                    unsigned char id = data.is_override_proto() ?
                        data.proto() : payloadProtocolId(ProtocolIdIp);
                    return id;
                }
                case FieldFrameValue:
                {
                    unsigned char id = data.is_override_proto() ?
                        data.proto() : payloadProtocolId(ProtocolIdIp);
                    return QByteArray(1, (char) id);
                }
                case FieldTextValue:
                {
                    unsigned char id = data.is_override_proto() ?
                        data.proto() : payloadProtocolId(ProtocolIdIp);
                    return  QString("0x%1").
                        arg(id, 2, BASE_HEX, QChar('0'));
                }
                default:
                    break;
            }
            break;
        }
        case ip4_cksum:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Header Checksum");
                case FieldValue:
                {
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumIp);
                    return cksum;
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumIp);

                    fv.resize(2);
                    qToBigEndian((quint16) cksum, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                {
                    quint16 cksum;

                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumIp);
                    return  QString("0x%1").
                        arg(cksum, 4, BASE_HEX, QChar('0'));;
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case ip4_srcAddr:
        {
            int        u;
            quint32    subnet, host, srcIp = 0;

            switch(data.src_ip_mode())
            {
                case OstProto::Ip4::e_im_fixed:
                    srcIp = data.src_ip();
                    break;
                case OstProto::Ip4::e_im_inc_host:
                    u = streamIndex % data.src_ip_count();
                    subnet = data.src_ip() & data.src_ip_mask();
                    host = (((data.src_ip() & ~data.src_ip_mask()) + u) &
                        ~data.src_ip_mask());
                    srcIp = subnet | host;
                    break;
                case OstProto::Ip4::e_im_dec_host:
                    u = streamIndex % data.src_ip_count();
                    subnet = data.src_ip() & data.src_ip_mask();
                    host = (((data.src_ip() & ~data.src_ip_mask()) - u) &
                        ~data.src_ip_mask());
                    srcIp = subnet | host;
                    break;
                case OstProto::Ip4::e_im_random_host:
                    subnet = data.src_ip() & data.src_ip_mask();
                    host = (qrand() & ~data.src_ip_mask());
                    srcIp = subnet | host;
                    break;
                default:
                    qWarning("Unhandled src_ip_mode = %d", data.src_ip_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Source");
                case FieldValue:
                    return srcIp;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian(srcIp, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QHostAddress(srcIp).toString();
                default:
                    break;
            }
            break;
        }
        case ip4_dstAddr:
        {
            int        u;
            quint32    subnet, host, dstIp = 0;

            switch(data.dst_ip_mode())
            {
                case OstProto::Ip4::e_im_fixed:
                    dstIp = data.dst_ip();
                    break;
                case OstProto::Ip4::e_im_inc_host:
                    u = streamIndex % data.dst_ip_count();
                    subnet = data.dst_ip() & data.dst_ip_mask();
                    host = (((data.dst_ip() & ~data.dst_ip_mask()) + u) &
                        ~data.dst_ip_mask());
                    dstIp = subnet | host;
                    break;
                case OstProto::Ip4::e_im_dec_host:
                    u = streamIndex % data.dst_ip_count();
                    subnet = data.dst_ip() & data.dst_ip_mask();
                    host = (((data.dst_ip() & ~data.dst_ip_mask()) - u) &
                        ~data.dst_ip_mask());
                    dstIp = subnet | host;
                    break;
                case OstProto::Ip4::e_im_random_host:
                    subnet = data.dst_ip() & data.dst_ip_mask();
                    host = (qrand() & ~data.dst_ip_mask());
                    dstIp = subnet | host;
                    break;
                default:
                    qWarning("Unhandled dst_ip_mode = %d", data.dst_ip_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination");
                case FieldValue:
                    return dstIp;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) dstIp, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QHostAddress(dstIp).toString();
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case ip4_isOverrideVer:
            switch(attrib)
            {
                case FieldValue: return data.is_override_ver();
                default: break;
            }
            break;
        case ip4_isOverrideHdrLen:
            switch(attrib)
            {
                case FieldValue: return data.is_override_hdrlen();
                default: break;
            }
            break;
        case ip4_isOverrideTotLen:
            switch(attrib)
            {
                case FieldValue: return data.is_override_totlen();
                default: break;
            }
            break;
        case ip4_isOverrideProto:
            switch(attrib)
            {
                case FieldValue: return data.is_override_proto();
                default: break;
            }
            break;
        case ip4_isOverrideCksum:
            switch(attrib)
            {
                case FieldValue: return data.is_override_cksum();
                default: break;
            }
            break;

        case ip4_srcAddrMode:
            switch(attrib)
            {
                case FieldValue: return data.src_ip_mode();
                default: break;
            }
            break;
        case ip4_srcAddrCount:
            switch(attrib)
            {
                case FieldValue: return data.src_ip_count();
                default: break;
            }
            break;
        case ip4_srcAddrMask:
            switch(attrib)
            {
                case FieldValue: return data.src_ip_mask();
                default: break;
            }
            break;

        case ip4_dstAddrMode:
            switch(attrib)
            {
                case FieldValue: return data.dst_ip_mode();
                default: break;
            }
            break;
        case ip4_dstAddrCount:
            switch(attrib)
            {
                case FieldValue: return data.dst_ip_count();
                default: break;
            }
            break;
        case ip4_dstAddrMask:
            switch(attrib)
            {
                case FieldValue: return data.dst_ip_mask();
                default: break;
            }
            break;
        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool Ip4Protocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case ip4_ver:
        {
            uint version = value.toUInt(&isOk);
            if (isOk)
                data.set_ver_hdrlen(
                        ((version & 0xF) << 4)
                      | (data.ver_hdrlen() & 0x0F));
            break;
        }
        case ip4_hdrLen:
        {
            uint hdrLen = value.toUInt(&isOk);
            if (isOk)
                data.set_ver_hdrlen(
                        (data.ver_hdrlen() & 0xF0)
                      | (hdrLen & 0x0F));
            break;
        }
        case ip4_tos:
        {
            uint tos = value.toUInt(&isOk);
            if (isOk)
                data.set_tos(tos);
            break;
        }
        case ip4_totLen:
        {
            uint totLen = value.toUInt(&isOk);
            if (isOk)
                data.set_totlen(totLen);
            break;
        }
        case ip4_id:
        {
            uint id = value.toUInt(&isOk);
            if (isOk)
                data.set_id(id);
            break;
        }
        case ip4_flags:
        {
            uint flags = value.toUInt(&isOk);
            if (isOk)
                data.set_flags(flags);
            break;
        }
        case ip4_fragOfs:
        {
            uint fragOfs = value.toUInt(&isOk);
            if (isOk)
                data.set_frag_ofs(fragOfs);
            break;
        }
        case ip4_ttl:
        {
            uint ttl = value.toUInt(&isOk);
            if (isOk)
                data.set_ttl(ttl);
            break;
        }
        case ip4_proto:
        {
            uint proto = value.toUInt(&isOk);
            if (isOk)
                data.set_proto(proto);
            break;
        }
        case ip4_cksum:
        {
            uint cksum = value.toUInt(&isOk);
            if (isOk)
                data.set_cksum(cksum);
            break;
        }
        case ip4_srcAddr:
        {
            quint32 srcIp = value.toUInt(&isOk);
            if (isOk)
                data.set_src_ip(srcIp);
            break;
        }
        case ip4_dstAddr:
        {
            quint32 dstIp = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_ip(dstIp);
            break;
        }

        // Meta-fields
        case ip4_isOverrideVer:
        {
            bool ovr = value.toBool();
            data.set_is_override_ver(ovr);
            isOk = true;
            break;
        }
        case ip4_isOverrideHdrLen:
        {
            bool ovr = value.toBool();
            data.set_is_override_hdrlen(ovr);
            isOk = true;
            break;
        }
        case ip4_isOverrideTotLen:
        {
            bool ovr = value.toBool();
            data.set_is_override_totlen(ovr);
            isOk = true;
            break;
        }
        case ip4_isOverrideProto:
        {
            bool ovr = value.toBool();
            data.set_is_override_proto(ovr);
            isOk = true;
            break;
        }
        case ip4_isOverrideCksum:
        {
            bool ovr = value.toBool();
            data.set_is_override_cksum(ovr);
            isOk = true;
            break;
        }

        case ip4_srcAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.IpAddrMode_IsValid(mode))
                data.set_src_ip_mode(OstProto::Ip4::IpAddrMode(mode));
            else
                isOk = false;
            break;
        }
        case ip4_srcAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_src_ip_count(count);
            break;
        }
        case ip4_srcAddrMask:
        {
            quint32 mask = value.toUInt(&isOk);
            if (isOk)
                data.set_src_ip_mask(mask);
            break;
        }

        case ip4_dstAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.IpAddrMode_IsValid(mode))
                data.set_dst_ip_mode(OstProto::Ip4::IpAddrMode(mode));
            else
                isOk = false;
            break;
        }
        case ip4_dstAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_ip_count(count);
            break;
        }
        case ip4_dstAddrMask:
        {
            quint32 mask = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_ip_mask(mask);
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

bool Ip4Protocol::isProtocolFrameValueVariable() const
{
    if ((data.src_ip_mode() != OstProto::Ip4::e_im_fixed) 
            || (data.dst_ip_mode() != OstProto::Ip4::e_im_fixed))
        return true;
    else
        return false;
}

int Ip4Protocol::protocolFrameVariableCount() const
{
    int count = 1;

    if (data.src_ip_mode() != OstProto::Ip4::e_im_fixed)
        count = AbstractProtocol::lcm(count, data.src_ip_count());

    if (data.dst_ip_mode() != OstProto::Ip4::e_im_fixed)
        count = AbstractProtocol::lcm(count, data.dst_ip_count());

    return count;
}

quint32 Ip4Protocol::protocolFrameCksum(int streamIndex,
    CksumType cksumType) const
{
    switch (cksumType)
    {
        case CksumIpPseudo:
        {
            quint32 sum;

            sum = fieldData(ip4_srcAddr, FieldValue, streamIndex).toUInt() >> 16;
            sum += fieldData(ip4_srcAddr, FieldValue, streamIndex).toUInt() & 0xFFFF;
            sum += fieldData(ip4_dstAddr, FieldValue, streamIndex).toUInt() >> 16;
            sum += fieldData(ip4_dstAddr, FieldValue, streamIndex).toUInt() & 0xFFFF;

            sum += fieldData(ip4_proto, FieldValue, streamIndex).toUInt() & 0x00FF;
            sum += (fieldData(ip4_totLen, FieldValue, streamIndex).toUInt() & 0xFFFF) - 20;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            // Above calculation done assuming 'big endian' 
            // - so convert to host order
            //return qFromBigEndian(sum);
            return ~sum;
        }
        default:
            break;
    }

    return AbstractProtocol::protocolFrameCksum(streamIndex, cksumType);
}
