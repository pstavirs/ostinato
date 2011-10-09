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

#include "ip6.h"

#include "ipv6addressvalidator.h"

#include <QHostAddress>
#include <qendian.h>

Ip6ConfigForm::Ip6ConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    version->setValidator(new QIntValidator(0, 0xF, this));
    payloadLength->setValidator(new QIntValidator(0, 0xFFFF, this));
    hopLimit->setValidator(new QIntValidator(0, 0xFF, this));

    srcAddr->setValidator(new IPv6AddressValidator(this));
    srcAddrCount->setValidator(new QIntValidator(this));
    //srcAddrPrefix->setValidator(new QIntValidator(0, 128, this));

    dstAddr->setValidator(new IPv6AddressValidator(this));
    dstAddrCount->setValidator(new QIntValidator(this));
    //dstAddrPrefix->setValidator(new QIntValidator(0, 128, this));
}

void Ip6ConfigForm::on_srcAddr_editingFinished()
{
    srcAddr->setText(QHostAddress(srcAddr->text()).toString());
}

void Ip6ConfigForm::on_dstAddr_editingFinished()
{
    dstAddr->setText(QHostAddress(dstAddr->text()).toString());
}

void Ip6ConfigForm::on_srcAddrModeCombo_currentIndexChanged(int index)
{
    bool enabled = (index > 0);

    srcAddrCount->setEnabled(enabled);
    srcAddrPrefix->setEnabled(enabled);
}

void Ip6ConfigForm::on_dstAddrModeCombo_currentIndexChanged(int index)
{
    bool enabled = (index > 0);

    dstAddrCount->setEnabled(enabled);
    dstAddrPrefix->setEnabled(enabled);
}

Ip6Protocol::Ip6Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    /* The configWidget is created lazily */
    configForm = NULL;
}

Ip6Protocol::~Ip6Protocol()
{
    delete configForm;
}

AbstractProtocol* Ip6Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Ip6Protocol(stream, parent);
}

quint32 Ip6Protocol::protocolNumber() const
{
    return OstProto::Protocol::kIp6FieldNumber;
}

void Ip6Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::ip6)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Ip6Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::ip6))
        data.MergeFrom(protocol.GetExtension(OstProto::ip6));
}

QString Ip6Protocol::name() const
{
    return QString("Internet Protocol ver 6");
}

QString Ip6Protocol::shortName() const
{
    return QString("IPv6");
}

AbstractProtocol::ProtocolIdType Ip6Protocol::protocolIdType() const
{
    return ProtocolIdIp;
}

quint32 Ip6Protocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdEth: return 0x86dd;
        case ProtocolIdIp: return 0x29;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int Ip6Protocol::fieldCount() const
{
    return ip6_fieldCount;
}

AbstractProtocol::FieldFlags Ip6Protocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case ip6_version:
        case ip6_trafficClass:
        case ip6_flowLabel:
        case ip6_payloadLength:
        case ip6_nextHeader:
        case ip6_hopLimit:
        case ip6_srcAddress:
        case ip6_dstAddress:
            break;

        case ip6_isOverrideVersion:
        case ip6_isOverridePayloadLength:
        case ip6_isOverrideNextHeader:

        case ip6_srcAddrMode:
        case ip6_srcAddrCount:
        case ip6_srcAddrPrefix:

        case ip6_dstAddrMode:
        case ip6_dstAddrCount:
        case ip6_dstAddrPrefix:
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

QVariant Ip6Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case ip6_version:
        {
            quint8 ver;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_version())
                        ver = data.version() & 0xF;
                    else
                        ver = 0x6;
                    break;
                default:
                    ver = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Version");
                case FieldValue:
                    return ver;
                case FieldTextValue:
                    return QString("%1").arg(ver);
                case FieldFrameValue:
                    return QByteArray(1, char(ver));
                case FieldBitSize:
                    return 4;
                default:
                    break;
            }
            break;
        }
        case ip6_trafficClass:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Traffic Class");
                case FieldValue:
                    return data.traffic_class() & 0xFF;
                case FieldTextValue:
                    return QString("%1").arg(data.traffic_class() & 0xFF, 
                            2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, char(data.traffic_class() & 0xFF));
                default:
                    break;
            }
            break;
        }
        case ip6_flowLabel:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Flow Label");
                case FieldValue:
                    return data.flow_label() & 0xFFFFF;
                case FieldTextValue:
                    return QString("%1").arg(data.flow_label() & 0xFFFFF, 
                            5, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.flow_label() & 0xFFFFF, 
                            (uchar*) fv.data());
                    fv = fv.right(3);
                    return fv;
                }
                case FieldBitSize:
                    return 20;
                default:
                    break;
            }
            break;
        }
        case ip6_payloadLength:
        {
            quint16 len;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_payload_length())
                        len = data.payload_length();
                    else
                        len = protocolFramePayloadSize(streamIndex);
                    break;
                default:
                    len = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Payload Length");
                case FieldValue:
                    return len;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(len, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("%1").arg(len);
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case ip6_nextHeader:
        {
            quint8 nextHdr;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_next_header())
                        nextHdr = data.next_header();
                    else
                        nextHdr = payloadProtocolId(ProtocolIdIp);
                    break;
                default:
                    nextHdr = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            switch(attrib)
            {
                case FieldName:            
                    return QString("Next Header");
                case FieldValue:
                    return nextHdr;
                case FieldTextValue:
                    return QString("%1").arg(nextHdr, 2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, char(nextHdr));
                default:
                    break;
            }
            break;
        }
        case ip6_hopLimit:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Hop Limit");
                case FieldValue:
                    return data.hop_limit() & 0xFF;
                case FieldTextValue:
                    return QString("%1").arg(data.hop_limit() & 0xFF);
                case FieldFrameValue:
                    return QByteArray(1, char(data.hop_limit() & 0xFF));
                default:
                    break;
            }
            break;
        }

        case ip6_srcAddress:
        {
            int u, p, q;
            quint64 maskHi = 0, maskLo = 0;
            quint64 prefixHi, prefixLo;
            quint64 hostHi = 0, hostLo = 0;
            quint64 srcHi = 0, srcLo = 0;

            switch(data.src_addr_mode())
            {
                case OstProto::Ip6::kFixed:
                    srcHi = data.src_addr_hi();
                    srcLo = data.src_addr_lo();
                    break;
                case OstProto::Ip6::kIncHost:
                case OstProto::Ip6::kDecHost:
                case OstProto::Ip6::kRandomHost:
                    u = streamIndex % data.src_addr_count();
                    if (data.src_addr_prefix() > 64) {
                        p = 64;
                        q = data.src_addr_prefix() - 64;
                    } else {
                        p = data.src_addr_prefix();
                        q = 0;
                    }
                    if (p > 0) 
                        maskHi = ~((quint64(1) << p) - 1);
                    if (q > 0) 
                        maskLo = ~((quint64(1) << q) - 1);
                    prefixHi = data.src_addr_hi() & maskHi;
                    prefixLo = data.src_addr_lo() & maskLo;
                    if (data.src_addr_mode() == OstProto::Ip6::kIncHost) {
                        hostHi = ((data.src_addr_hi() & ~maskHi) + u) & ~maskHi;
                        hostLo = ((data.src_addr_lo() & ~maskLo) + u) & ~maskLo;
                    } 
                    else if (data.src_addr_mode() == OstProto::Ip6::kDecHost) {
                        hostHi = ((data.src_addr_hi() & ~maskHi) - u) & ~maskHi;
                        hostLo = ((data.src_addr_lo() & ~maskLo) - u) & ~maskLo;
                    } 
                    else if (data.src_addr_mode()==OstProto::Ip6::kRandomHost) {
                        hostHi = qrand() & ~maskHi;
                        hostLo = qrand() & ~maskLo;
                    }
                    srcHi = prefixHi | hostHi;
                    srcLo = prefixLo | hostLo;
                    break;
                default:
                    qWarning("Unhandled src_addr_mode = %d", 
                            data.src_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Source");
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                {
                    QByteArray fv;
                    fv.resize(16);
                    qToBigEndian(srcHi, (uchar*) fv.data());
                    qToBigEndian(srcLo, (uchar*) (fv.data() + 8));
                    if (attrib == FieldTextValue)
                        return QHostAddress((quint8*)fv.constData()).toString();
                    else
                        return fv;
                }
                default:
                    break;
            }
            break;
        }

        case ip6_dstAddress:
        {
            int u, p, q;
            quint64 maskHi = 0, maskLo = 0;
            quint64 prefixHi, prefixLo;
            quint64 hostHi = 0, hostLo = 0;
            quint64 dstHi = 0, dstLo = 0;

            switch(data.dst_addr_mode())
            {
                case OstProto::Ip6::kFixed:
                    dstHi = data.dst_addr_hi();
                    dstLo = data.dst_addr_lo();
                    break;
                case OstProto::Ip6::kIncHost:
                case OstProto::Ip6::kDecHost:
                case OstProto::Ip6::kRandomHost:
                    u = streamIndex % data.dst_addr_count();
                    if (data.dst_addr_prefix() > 64) {
                        p = 64;
                        q = data.dst_addr_prefix() - 64;
                    } else {
                        p = data.dst_addr_prefix();
                        q = 0;
                    }
                    if (p > 0) 
                        maskHi = ~((quint64(1) << p) - 1);
                    if (q > 0) 
                        maskLo = ~((quint64(1) << q) - 1);
                    prefixHi = data.dst_addr_hi() & maskHi;
                    prefixLo = data.dst_addr_lo() & maskLo;
                    if (data.dst_addr_mode() == OstProto::Ip6::kIncHost) {
                        hostHi = ((data.dst_addr_hi() & ~maskHi) + u) & ~maskHi;
                        hostLo = ((data.dst_addr_lo() & ~maskLo) + u) & ~maskLo;
                    } 
                    else if (data.dst_addr_mode() == OstProto::Ip6::kDecHost) {
                        hostHi = ((data.dst_addr_hi() & ~maskHi) - u) & ~maskHi;
                        hostLo = ((data.dst_addr_lo() & ~maskLo) - u) & ~maskLo;
                    } 
                    else if (data.dst_addr_mode()==OstProto::Ip6::kRandomHost) {
                        hostHi = qrand() & ~maskHi;
                        hostLo = qrand() & ~maskLo;
                    }
                    dstHi = prefixHi | hostHi;
                    dstLo = prefixLo | hostLo;
                    break;
                default:
                    qWarning("Unhandled dst_addr_mode = %d", 
                            data.dst_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination");
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                {
                    QByteArray fv;
                    fv.resize(16);
                    qToBigEndian(dstHi, (uchar*) fv.data());
                    qToBigEndian(dstLo, (uchar*) (fv.data() + 8));
                    if (attrib == FieldTextValue)
                        return QHostAddress((quint8*)fv.constData()).toString();
                    else
                        return fv;
                }
                default:
                    break;
            }
            break;
        }

        // Meta-Fields
        case ip6_isOverrideVersion:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_version();
                default:
                    break;
            }
            break;
        }
        case ip6_isOverridePayloadLength:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_payload_length();
                default:
                    break;
            }
            break;
        }
        case ip6_isOverrideNextHeader:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_next_header();
                default:
                    break;
            }
            break;
        }

        case ip6_srcAddrMode:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_mode();
                default:
                    break;
            }
            break;
        }
        case ip6_srcAddrCount:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_count();
                default:
                    break;
            }
            break;
        }
        case ip6_srcAddrPrefix:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.src_addr_prefix();
                default:
                    break;
            }
            break;
        }

        case ip6_dstAddrMode:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_mode();
                default:
                    break;
            }
            break;
        }
        case ip6_dstAddrCount:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_count();
                default:
                    break;
            }
            break;
        }
        case ip6_dstAddrPrefix:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.dst_addr_prefix();
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

bool Ip6Protocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case ip6_version:
        {
            uint ver = value.toUInt(&isOk);
            if (isOk)
                data.set_version(ver & 0xF);
            break;
        }
        case ip6_trafficClass:
        {
            uint trfClass = value.toUInt(&isOk);
            if (isOk)
                data.set_traffic_class(trfClass & 0xFF);
            break;
        }
        case ip6_flowLabel:
        {
            uint fl = value.toUInt(&isOk);
            if (isOk)
                data.set_flow_label(fl & 0xFFFFF);
            break;
        }
        case ip6_payloadLength:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_payload_length(len & 0xFFFF);
            break;
        }
        case ip6_nextHeader:
        {
            uint ver = value.toUInt(&isOk);
            if (isOk)
                data.set_next_header(ver & 0xFF);
            break;
        }
        case ip6_hopLimit:
        {
            uint hl = value.toUInt(&isOk);
            if (isOk)
                data.set_hop_limit(hl & 0xFF);
            break;
        }
        case ip6_srcAddress:
        {
            Q_IPV6ADDR addr = QHostAddress(value.toString()).toIPv6Address();
            quint64 x;

            x =   (quint64(addr[0]) << 56)
                | (quint64(addr[1]) << 48)
                | (quint64(addr[2]) << 40)
                | (quint64(addr[3]) << 32)
                | (quint64(addr[4]) << 24)
                | (quint64(addr[5]) << 16)
                | (quint64(addr[6]) <<  8)
                | (quint64(addr[7]) <<  0);
            data.set_src_addr_hi(x);

            x =   (quint64(addr[ 8]) << 56)
                | (quint64(addr[ 9]) << 48)
                | (quint64(addr[10]) << 40)
                | (quint64(addr[11]) << 32)
                | (quint64(addr[12]) << 24)
                | (quint64(addr[13]) << 16)
                | (quint64(addr[14]) <<  8)
                | (quint64(addr[15]) <<  0);
            data.set_src_addr_lo(x);
            break;
        }
        case ip6_dstAddress:
        {
            Q_IPV6ADDR addr = QHostAddress(value.toString()).toIPv6Address();
            quint64 x;

            x =   (quint64(addr[0]) << 56)
                | (quint64(addr[1]) << 48)
                | (quint64(addr[2]) << 40)
                | (quint64(addr[3]) << 32)
                | (quint64(addr[4]) << 24)
                | (quint64(addr[5]) << 16)
                | (quint64(addr[6]) <<  8)
                | (quint64(addr[7]) <<  0);
            data.set_dst_addr_hi(x);

            x =   (quint64(addr[ 8]) << 56)
                | (quint64(addr[ 9]) << 48)
                | (quint64(addr[10]) << 40)
                | (quint64(addr[11]) << 32)
                | (quint64(addr[12]) << 24)
                | (quint64(addr[13]) << 16)
                | (quint64(addr[14]) <<  8)
                | (quint64(addr[15]) <<  0);
            data.set_dst_addr_lo(x);
            break;
        }

        // Meta-Fields
        case ip6_isOverrideVersion:
        {
            bool ovr = value.toBool();
            data.set_is_override_version(ovr);
            isOk = true;
            break;
        }
        case ip6_isOverridePayloadLength:
        {
            bool ovr = value.toBool();
            data.set_is_override_payload_length(ovr);
            isOk = true;
            break;
        }
        case ip6_isOverrideNextHeader:
        {
            bool ovr = value.toBool();
            data.set_is_override_next_header(ovr);
            isOk = true;
            break;
        }

        case ip6_srcAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.AddrMode_IsValid(mode))
                data.set_src_addr_mode((OstProto::Ip6::AddrMode) mode);
            else
                isOk = false;
            break;
        }
        case ip6_srcAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_src_addr_count(count);
            break;
        }
        case ip6_srcAddrPrefix:
        {
            uint prefix = value.toUInt(&isOk);
            if (isOk)
                data.set_src_addr_prefix(prefix);
            break;
        }

        case ip6_dstAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.AddrMode_IsValid(mode))
                data.set_dst_addr_mode((OstProto::Ip6::AddrMode) mode);
            else
                isOk = false;
            break;
        }
        case ip6_dstAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_addr_count(count);
            break;
        }
        case ip6_dstAddrPrefix:
        {
            uint prefix = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_addr_prefix(prefix);
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

bool Ip6Protocol::isProtocolFrameValueVariable() const
{
    if ((data.src_addr_mode() != OstProto::Ip6::kFixed)
        || (data.dst_addr_mode() != OstProto::Ip6::kFixed))
        return true;
    else
        return false;
}

int Ip6Protocol::protocolFrameVariableCount() const
{
    int count = 1;

    if (data.src_addr_mode() != OstProto::Ip6::kFixed)
        count = AbstractProtocol::lcm(count, data.src_addr_count());

    if (data.dst_addr_mode() != OstProto::Ip6::kFixed)
        count = AbstractProtocol::lcm(count, data.dst_addr_count());

    return count;
}

quint32 Ip6Protocol::protocolFrameCksum(int streamIndex, 
        CksumType cksumType) const
{
    if (cksumType == CksumIpPseudo)
    {
        QByteArray addr;
        quint32 sum = 0;

        addr = fieldData(ip6_srcAddress, FieldFrameValue, streamIndex)
                .toByteArray();
        Q_ASSERT(addr.size() == 16);
        for (int i = 0; i < addr.size(); i+=2)
            sum += (quint8(addr.at(i)) << 8) + quint8(addr.at(i+1));

        addr = fieldData(ip6_dstAddress, FieldFrameValue, streamIndex)
                .toByteArray();
        Q_ASSERT(addr.size() == 16);
        for (int i = 0; i < addr.size(); i+=2)
            sum += (quint8(addr.at(i)) << 8) + quint8(addr.at(i+1));

        sum += fieldData(ip6_payloadLength, FieldValue, streamIndex)
                .toUInt() & 0xFFFF;
        sum += fieldData(ip6_nextHeader, FieldValue, streamIndex)
                .toUInt() & 0xFF;

        while(sum>>16)
            sum = (sum & 0xFFFF) + (sum >> 16);

        return ~sum;
    }
    return AbstractProtocol::protocolFrameCksum(streamIndex, cksumType);
}

QWidget* Ip6Protocol::configWidget()
{
    /* Lazy creation of the configWidget */
    if (configForm == NULL)
    {
        configForm = new Ip6ConfigForm;
        loadConfigWidget();
    }

    return configForm;
}

void Ip6Protocol::loadConfigWidget()
{
    configWidget();

    configForm->isVersionOverride->setChecked(
        fieldData(ip6_isOverrideVersion, FieldValue).toBool());
    configForm->version->setText(
        fieldData(ip6_version, FieldValue).toString());

    configForm->trafficClass->setText(uintToHexStr(
        fieldData(ip6_trafficClass, FieldValue).toUInt(), 1));

    configForm->flowLabel->setText(QString("%1").arg(
        fieldData(ip6_flowLabel, FieldValue).toUInt(),5, BASE_HEX, QChar('0')));

    configForm->isPayloadLengthOverride->setChecked(
        fieldData(ip6_isOverridePayloadLength, FieldValue).toBool());
    configForm->payloadLength->setText(
        fieldData(ip6_payloadLength, FieldValue).toString());

    configForm->isNextHeaderOverride->setChecked(
        fieldData(ip6_isOverrideNextHeader, FieldValue).toBool());
    configForm->nextHeader->setText(uintToHexStr(
        fieldData(ip6_nextHeader, FieldValue).toUInt(), 1));

    configForm->hopLimit->setText(
        fieldData(ip6_hopLimit, FieldValue).toString());

    configForm->srcAddr->setText(
        fieldData(ip6_srcAddress, FieldTextValue).toString());
    configForm->srcAddrModeCombo->setCurrentIndex(
        fieldData(ip6_srcAddrMode, FieldValue).toUInt());
    configForm->srcAddrCount->setText(
        fieldData(ip6_srcAddrCount, FieldValue).toString());
    configForm->srcAddrPrefix->setText(
        fieldData(ip6_srcAddrPrefix, FieldValue).toString());

    configForm->dstAddr->setText(
        fieldData(ip6_dstAddress, FieldTextValue).toString());
    configForm->dstAddrModeCombo->setCurrentIndex(
        fieldData(ip6_dstAddrMode, FieldValue).toUInt());
    configForm->dstAddrCount->setText(
        fieldData(ip6_dstAddrCount, FieldValue).toString());
    configForm->dstAddrPrefix->setText(
        fieldData(ip6_dstAddrPrefix, FieldValue).toString());
}

void Ip6Protocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    setFieldData(ip6_isOverrideVersion, 
        configForm->isVersionOverride->isChecked());
    setFieldData(ip6_version, configForm->version->text());

    setFieldData(ip6_trafficClass, 
        configForm->trafficClass->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    setFieldData(ip6_flowLabel, 
        configForm->flowLabel->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    setFieldData(ip6_isOverridePayloadLength, 
        configForm->isPayloadLengthOverride->isChecked());
    setFieldData(ip6_payloadLength, configForm->payloadLength->text());

    setFieldData(ip6_isOverrideNextHeader, 
        configForm->isNextHeaderOverride->isChecked());
    setFieldData(ip6_nextHeader, 
        configForm->nextHeader->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    setFieldData(ip6_hopLimit, configForm->hopLimit->text());

    setFieldData(ip6_srcAddress, configForm->srcAddr->text());
    setFieldData(ip6_srcAddrMode, configForm->srcAddrModeCombo->currentIndex());
    setFieldData(ip6_srcAddrCount, configForm->srcAddrCount->text());
    setFieldData(ip6_srcAddrPrefix, configForm->srcAddrPrefix->text());

    setFieldData(ip6_dstAddress, configForm->dstAddr->text());
    setFieldData(ip6_dstAddrMode, configForm->dstAddrModeCombo->currentIndex());
    setFieldData(ip6_dstAddrCount, configForm->dstAddrCount->text());
    setFieldData(ip6_dstAddrPrefix, configForm->dstAddrPrefix->text());
}

