/*
Copyright (C) 2014 PLVision.

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

This module is developed by PLVision  <developers@plvision.eu>
*/

#include "stp.h"
#include <QRegExp>

#define uintToMacStr(num)    \
    QString("%1").arg(num, 6 * 2, BASE_HEX, QChar('0')) \
    .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper()
#define ONE_BIT(pos) ((unsigned int)(1 << (pos)))
#define BITS(bit) (bit)
#define BYTES(byte) (byte)
#define BYTES_TO_BITS(byte) (byte * 8)

#define STP_LLC 0x424203

StpProtocol::StpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

StpProtocol::~StpProtocol()
{
}

AbstractProtocol* StpProtocol::createInstance(StreamBase *stream,
                                              AbstractProtocol *parent)
{
    return new StpProtocol(stream, parent);
}

quint32 StpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kStpFieldNumber;
}

void StpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::stp)->CopyFrom(data_);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void StpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
        protocol.HasExtension(OstProto::stp))
        data_.MergeFrom(protocol.GetExtension(OstProto::stp));
}

QString StpProtocol::name() const
{
    return QString("Spanning Tree Protocol");
}

QString StpProtocol::shortName() const
{
    return QString("STP");
}

quint32 StpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdLlc:
            return STP_LLC;
        default:
            break;
    }

    return AbstractProtocol::protocolId(type);
}

int StpProtocol::fieldCount() const
{
    return stp_fieldCount;
}

AbstractProtocol::FieldFlags StpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case stp_protocol_id:
        case stp_version_id:
        case stp_bpdu_type:
        case stp_flags:
        case stp_root_id:
        case stp_root_path_cost:
        case stp_bridge_id:
        case stp_port_id:
        case stp_message_age:
        case stp_max_age:
        case stp_hello_time:
        case stp_forward_delay:
            break;
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                   index);
            break;
    }
    return flags;
}

QVariant StpProtocol::fieldData(int index, FieldAttrib attrib,
                                int streamIndex) const
{
    QString str[] = {"Topology Change", "Topology Change Acknowledgment"};

    switch (index)
    {
        case stp_protocol_id:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Protocol Identifier");
                case FieldValue:
                    return data_.protocol_id();
                case FieldTextValue:
                    return QString("0x%1").arg(data_.protocol_id(),
                                               4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data_.protocol_id(),
                                 (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        case stp_version_id:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Version Identifier");
                case FieldValue:
                    return data_.protocol_version_id();
                case FieldTextValue:
                    return QString("%1").arg(
                                data_.protocol_version_id());
                case FieldFrameValue:
                    return QByteArray(1,
                                      (char)data_.protocol_version_id());
                case FieldBitSize:
                    return BYTES_TO_BITS(1);
                default:
                    break;
            }
            break;
        }
        case stp_bpdu_type:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("BPDU Type");
                case FieldValue:
                    return data_.bpdu_type();
                case FieldTextValue:
                    return QString("0x%1").arg(data_.bpdu_type(),
                                               2, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)data_.bpdu_type());
                case FieldBitSize:
                    return BYTES_TO_BITS(1);
                default:
                    break;
            }
            break;
        }
        case stp_flags:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("BPDU Flags");
                case FieldValue:
                    return data_.flags();
                case FieldTextValue:
                {
                    QString strTemp = "(";
                    if((data_.flags() & ONE_BIT(0))) strTemp += str[0] + ", ";
                    if((data_.flags() & ONE_BIT(7))) strTemp += str[1] + ", ";
                    strTemp += ")";
                    strTemp.replace(", )", ")");
                    return strTemp;
                }
                case FieldFrameValue:
                    return QByteArray(1, (char)data_.flags());
                case FieldBitSize:
                    return BYTES_TO_BITS(1);
                default:
                    break;
            }
            break;
        }
        case stp_root_id:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Root Identifier");
                case FieldValue:
                    return (quint64) data_.root_id();
                case FieldTextValue:
                {
                    // Root ID contain two value:
                    // Root ID Priority(first 2 bytes)
                    // and Root ID MAC (last 6 bytes). (IEEE802.1D-2008)
                    quint16 priority = (
                        data_.root_id() & 0xFFFF000000000000ULL) >> (BYTES_TO_BITS(6));
                    quint64 mac = data_.root_id() & 0x0000FFFFFFFFFFFFULL;
                    return QString("Priority: %1 / MAC: %2")
                            .arg(QString::number(priority),
                        uintToMacStr(mac));
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(8));
                    qToBigEndian((quint64)data_.root_id(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(8);
                default:
                    break;
            }
            break;
        }
        case stp_root_path_cost:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Root Path Cost");
                case FieldValue:
                    return data_.root_path_cost();
                case FieldTextValue:
                    return QString("%1").arg(data_.root_path_cost());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(4));
                    qToBigEndian(data_.root_path_cost(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(4);
                default:
                    break;
            }
            break;
        }
        case stp_bridge_id:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Bridge Identifier");
                case FieldValue:
                    return (quint64) data_.bridge_id();
                case FieldTextValue:
                {
                    // Bridge ID contain two value:
                    // Bridge ID Priority(first 2 bytes)
                    // and Bridge ID MAC (last 6 bytes). (IEEE802.1D-2008)
                    quint16 priority = (data_.bridge_id() & 0xFFFF000000000000ULL
                                        ) >> (BYTES_TO_BITS(6));
                    quint64 mac = data_.bridge_id() & 0x0000FFFFFFFFFFFFULL;
                    return QString("Priority: %1 / MAC: %2").arg(QString::number(priority),
                                                  uintToMacStr(mac));
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(8));
                    qToBigEndian((quint64)data_.bridge_id(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(8);
                default:
                    break;
            }
            break;
        }
        case stp_port_id:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Port Identifier");
                case FieldValue:
                    return data_.port_id();
                case FieldTextValue:
                    return QString("0x%1").arg(data_.port_id(), 4,
                                               BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data_.port_id(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        case stp_message_age:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Message Age");
                case FieldValue:
                    return data_.message_age();
                case FieldTextValue:
                    return QString("%1").arg(data_.message_age());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)(data_.message_age()),
                                 (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        case stp_max_age:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Max Age");
                case FieldValue:
                    return data_.max_age();
                case FieldTextValue:
                    return QString("%1").arg(data_.max_age());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data_.max_age(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        case stp_hello_time:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Hello Time");
                case FieldValue:
                    return data_.hello_time();
                case FieldTextValue:
                    return QString("%1").arg(data_.hello_time());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data_.hello_time(), (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        case stp_forward_delay:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Forward Delay");
                case FieldValue:
                    return data_.forward_delay();
                case FieldTextValue:
                    return QString("%1").arg(data_.forward_delay());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data_.forward_delay(),
                                 (uchar*)fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return BYTES_TO_BITS(2);
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool StpProtocol::setFieldData(int index, const QVariant &value,
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return isOk;

    switch (index)
    {
        case stp_protocol_id:
        {
            quint16 protoId = value.toUInt(&isOk);
            if (isOk)
                data_.set_protocol_id(protoId);
            break;
        }
        case stp_version_id:
        {
            quint8 versionId = value.toUInt(&isOk);
            if (isOk)
                data_.set_protocol_version_id(versionId);
            break;
        }
        case stp_bpdu_type:
        {
            quint8 bpdu = value.toUInt(&isOk);
            if (isOk)
                data_.set_bpdu_type(bpdu);
            break;
        }
        case stp_flags:
        {
            quint8 flags = value.toUInt(&isOk);
            if (isOk)
                data_.set_flags(flags);
            break;
        }
        case stp_root_id:
        {
            quint64 rootId = value.toULongLong(&isOk);
            if (isOk)
                data_.set_root_id(rootId);
            break;
        }
        case stp_root_path_cost:
        {
            quint32 pathCost = value.toUInt(&isOk);
            if (isOk)
                data_.set_root_path_cost(pathCost);
            break;
        }
        case stp_bridge_id:
        {
            quint64 bridgeId = value.toULongLong(&isOk);
            if (isOk)
                data_.set_bridge_id(bridgeId);
            break;
        }
        case stp_port_id:
        {
            quint32 port_id = value.toUInt(&isOk);
            if (isOk)
                data_.set_port_id(port_id);
            break;
        }
        case stp_message_age:
        {
            quint32 messageAge = value.toUInt(&isOk);
            if (isOk)
                data_.set_message_age(messageAge);
            break;
        }
        case stp_max_age:
        {
            quint32 maxAge = value.toUInt(&isOk);
            if (isOk)
                data_.set_max_age(maxAge);
            break;
        }
        case stp_hello_time:
        {
            quint32 helloTime = value.toUInt(&isOk);
            if (isOk)
                data_.set_hello_time(helloTime);
            break;
        }
        case stp_forward_delay:
        {
            quint32 forwardDelay = value.toUInt(&isOk);
            if (isOk)
                data_.set_forward_delay(forwardDelay);
            break;
        }
        default:
            break;
    }
    return isOk;
}
