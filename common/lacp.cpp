/*
Copyright (C) 2014 Marchuk S.

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

#include "lacp.h"
#include <QRegExp>

#define uintToMacStr(num)    \
    QString("%1").arg(num, 6 * 2, BASE_HEX, QChar('0')) \
    .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper()
#define ONE_BIT(pos) ((unsigned int)(1 << (pos)))
#define BITS(bit) (bit)
#define BYTES(byte) (byte)
#define BYTES_TO_BITS(byte) (byte * 8)

#define LLDP_ETHERTYPE 0x8809

LacpProtocol::LacpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

LacpProtocol::~LacpProtocol()
{
}

AbstractProtocol* LacpProtocol::createInstance(StreamBase *stream,
                                               AbstractProtocol *parent)
{
    return new LacpProtocol(stream, parent);
}

quint32 LacpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kLacpFieldNumber;
}

void LacpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::lacp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void LacpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
        protocol.HasExtension(OstProto::lacp))
        data.MergeFrom(protocol.GetExtension(OstProto::lacp));
}

QString LacpProtocol::name() const
{
    return QString("Link Aggregation Control Protocol");
}

QString LacpProtocol::shortName() const
{
    return QString("LACP");
}

AbstractProtocol::ProtocolIdType LacpProtocol::protocolIdType() const
{
    return ProtocolIdEth;
}

quint32 LacpProtocol::protocolId(ProtocolIdType type) const
{
    switch (type)
    {
        case ProtocolIdEth:
            return LLDP_ETHERTYPE;
        default:
            break;
    }

    return AbstractProtocol::protocolId(type);
}

int LacpProtocol::fieldCount() const
{
    return lacp_fieldCount;
}

AbstractProtocol::FieldFlags LacpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case lacp_subtype:
        case lacp_version_number:
        case lacp_tlv_type_actor:
        case lacp_actor_length:
        case lacp_actor_system_priority:
        case lacp_actor_system:
        case lacp_actor_key:
        case lacp_actor_port_priority:
        case lacp_actor_port:
        case lacp_actor_state:
        case lacp_actor_reserved:

        case lacp_tlv_type_partner:
        case lacp_partner_length:
        case lacp_partner_system_priority:
        case lacp_partner_system:
        case lacp_partner_key:
        case lacp_partner_port_priority:
        case lacp_partner_port:
        case lacp_partner_state:
        case lacp_partner_reserved:

        case lacp_tlv_type_collector:
        case lacp_collector_length:
        case lacp_collector_maxDelay:
        case lacp_collector_reserved:
        case lacp_tlv_type_terminator:
        case lacp_terminator_length:
        case lacp_terminator_reserved:
            break;

          // Meta Fields
        case is_override_subtype:
        case is_override_version:
        case is_override_tlv_actor:
        case is_override_actor_info:
        case is_override_actor_reserved:
        case is_override_tlv_partner:
        case is_override_partner_info:
        case is_override_partner_reserved:
        case is_override_tlv_collector:
        case is_override_collector_info:
        case is_override_collector_reserved:
        case is_override_tlv_terminator:
        case is_override_terminator_info:
        case is_override_terminator_reserved:
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

QVariant LacpProtocol::fieldData(int index, FieldAttrib attrib,
                                 int streamIndex) const
{
    QString str[8]={"Activity", "Timeout", "Aggregation", "Synchronization",
                    "Collecting", "Distributing", "Defaulted", "Expired"};

    switch (index)
    {
        case lacp_subtype:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Subtype");
                case FieldValue:
                    return data.subtype();
                case FieldTextValue:
                    return QString("0x%1").arg(data.subtype(), 2, BASE_HEX,
                                               QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)data.subtype());
                default:
                    break;
            }
            break;
        }
        case lacp_version_number:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Version Number");
                case FieldValue:
                    return data.version_number();
                case FieldTextValue:
                    return QString("0x%1").arg(data.version_number(), 2,
                                               BASE_HEX, QChar('0'));
                case FieldFrameValue:
                    return QByteArray(1, (char)data.version_number());
                default:
                    break;
            }
            break;
        }
//--------------------------------Actor-----------------------------------------
        case lacp_tlv_type_actor:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor Information");
                case FieldValue:
                    return data.tlv_type_actor();
                case FieldTextValue:
                    return QString("%1").arg(data.tlv_type_actor());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.tlv_type_actor());
                default:
                    break;
            }
            break;
        }
        case lacp_actor_length:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor Information Length");
                case FieldValue:
                    return data.actor_info_length();
                case FieldTextValue:
                    return QString("%1").arg(data.actor_info_length());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.actor_info_length());
                default:
                    break;
            }
            break;
        }
        case lacp_actor_system_priority:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor System Priority");
                case FieldValue:
                    return data.actor_system_priority();
                case FieldTextValue:
                    return QString("%1").arg(data.actor_system_priority());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.actor_system_priority(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_actor_system:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor System");
                case FieldValue:
                    return (quint64)data.actor_system();
                case FieldTextValue:
                    return uintToMacStr(data.actor_system());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    // 8 bytes because data.actor_system is UInt64
                    fv.resize(BYTES(8));
                    qToBigEndian(data.actor_system() , (uchar*)fv.data());
                    // remove first two bytes because actor system have 6 byte
                    // (IEEE802.1AX-2008)
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_actor_key:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor Key");
                case FieldValue:
                    return data.actor_key();
                case FieldTextValue:
                    return QString("%1").arg(data.actor_key());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.actor_key(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_actor_port_priority:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor Port Priority");
                case FieldValue:
                    return data.actor_port_priority();
                case FieldTextValue:
                    return QString("%1").arg(data.actor_port_priority());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.actor_port_priority(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_actor_port:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor Port");
                case FieldValue:
                    return data.actor_port();
                case FieldTextValue:
                    return QString("%1").arg(data.actor_port());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.actor_port(), (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_actor_state:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Actor State");
                case FieldValue:
                    return data.actor_state();
                case FieldTextValue:
                {
                    QString str_temp = "(";
                    if ((data.actor_state() & ONE_BIT(0)))
                        str_temp += str[0] + ", ";
                    if ((data.actor_state() & ONE_BIT(1)))
                        str_temp += str[1] + ", ";
                    if ((data.actor_state() & ONE_BIT(2)))
                        str_temp += str[2] + ", ";
                    if ((data.actor_state() & ONE_BIT(3)))
                        str_temp += str[3] + ", ";
                    if ((data.actor_state() & ONE_BIT(4)))
                        str_temp += str[4] + ", ";
                    if ((data.actor_state() & ONE_BIT(5)))
                        str_temp += str[5] + ", ";
                    if ((data.actor_state() & ONE_BIT(6)))
                        str_temp += str[6] + ", ";
                    if ((data.actor_state() & ONE_BIT(7)))
                        str_temp += str[7] + ", ";
                    str_temp += ")";
                    str_temp.replace(", )", ")");
                    return str_temp;
                }
                case FieldFrameValue:
                    return QByteArray(1, (char)data.actor_state());
                default:
                    break;
            }
            break;
        }
        case lacp_actor_reserved:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Reserved");
                case FieldValue:
                    return QString("%1").arg(data.actor_reserved(), 6,
                                             BASE_HEX, QChar('0'));
                case FieldTextValue:
                    return QString("0x%1").arg(data.actor_reserved(), 6,
                                               BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    // must have 4 bytes because data.actor_reserved is int32
                    fv.resize(BYTES(4));
                    qToBigEndian(data.actor_reserved(), (uchar*)fv.data());
                     // remove first byte, because field actor reserved
                     // have 3 byte (IEEE802.1AX-2008)
                    fv.remove(0, 1);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
//--------------------------------Partner---------------------------------------
        case lacp_tlv_type_partner:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner Information");
                case FieldValue:
                    return data.tlv_type_partner();
                case FieldTextValue:
                    return QString("%1").arg(data.tlv_type_partner());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.tlv_type_partner());
                default:
                    break;
            }
            break;
        }
        case lacp_partner_length:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner Information Length");
                case FieldValue:
                    return data.partner_info_length();
                case FieldTextValue:
                    return QString("%1").arg(data.partner_info_length());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.partner_info_length());
                default:
                    break;
            }
            break;
        }
        case lacp_partner_system_priority:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner System Priority");
                case FieldValue:
                    return data.partner_system_priority();
                case FieldTextValue:
                    return QString("%1").arg(data.partner_system_priority());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.partner_system_priority(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_partner_system:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner System");
                case FieldValue:
                    return (quint64)data.partner_system();
                case FieldTextValue:
                    return uintToMacStr(data.partner_system());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    // must have 8 bytes because data.partner_system is UInt64
                    fv.resize(BYTES(8));
                    qToBigEndian(data.partner_system() , (uchar*)fv.data());
                     // remove first byte, because field partner system
                     // have 6 byte (IEEE802.1AX-2008)
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_partner_key:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner Key");
                case FieldValue:
                    return (quint64)data.partner_key();
                case FieldTextValue:
                    return QString("%1").arg(data.partner_key());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.partner_key(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_partner_port_priority:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner Port Priority");
                case FieldValue:
                    return data.partner_port_priority();
                case FieldTextValue:
                    return QString("%1").arg(data.partner_port_priority());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.partner_port_priority(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_partner_port:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner Port");
                case FieldValue:
                    return data.partner_port();
                case FieldTextValue:
                    return QString("%1").arg(data.partner_port());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.partner_port(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_partner_state:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Partner State");
                case FieldValue:
                    return data.partner_state();
                case FieldTextValue:
                {
                    QString str_temp="(";
                    if ((data.partner_state() & ONE_BIT(0)))
                        str_temp += str[0] + ", ";
                    if ((data.partner_state() & ONE_BIT(1)))
                        str_temp += str[1] + ", ";
                    if ((data.partner_state() & ONE_BIT(2)))
                        str_temp += str[2] + ", ";
                    if ((data.partner_state() & ONE_BIT(3)))
                        str_temp += str[3] + ", ";
                    if ((data.partner_state() & ONE_BIT(4)))
                        str_temp += str[4] + ", ";
                    if ((data.partner_state() & ONE_BIT(5)))
                        str_temp += str[5] + ", ";
                    if ((data.partner_state() & ONE_BIT(6)))
                        str_temp += str[6] + ", ";
                    if ((data.partner_state() & ONE_BIT(7)))
                        str_temp += str[7] + ", ";
                    str_temp+=")";
                    str_temp.replace(", )", ")");
                    return str_temp;
                }
                case FieldFrameValue:
                    return QByteArray(1, (char)data.partner_state());
                default:
                    break;
            }
            break;
        }
        case lacp_partner_reserved:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Reserved");
                case FieldValue:
                    return data.partner_reserved();
                case FieldTextValue:
                    return QString("0x%1").arg(data.partner_reserved(), 6,
                                               BASE_HEX, QChar('0'));
               case FieldFrameValue:
                {
                    QByteArray fv;
                    // must have 4 bytes because data.partner_reserved is UInt32
                    fv.resize(4);
                    qToBigEndian(data.partner_reserved(), (uchar*)fv.data());
                    // remove first byte, because field partner reserved
                    // have 3 byte (IEEE802.1AX-2008)
                    fv.remove(0, 1);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
//--------------------------------------------Collector-------------------------
        case lacp_tlv_type_collector:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Collector Information");
                case FieldValue:
                    return data.tlv_type_collector();
                case FieldTextValue:
                    return QString("%1").arg(data.tlv_type_collector());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.tlv_type_collector());
                default:
                    break;
            }
            break;
        }
        case lacp_collector_length:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Collector Information Length");
                case FieldValue:
                    return data.collector_info_length();
                case FieldTextValue:
                    return QString("%1").arg(data.collector_info_length());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.collector_info_length());
                default:
                    break;
            }
            break;
        }
        case lacp_collector_maxDelay:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Collector Max Delay");
                case FieldValue:
                    return data.collector_maxdelay();
                case FieldTextValue:
                    return QString("%1").
                        arg(data.collector_maxdelay());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(BYTES(2));
                    qToBigEndian((quint16)data.collector_maxdelay(),
                                 (uchar*)fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case lacp_collector_reserved:
        {
            QByteArray ba;
            bool isOk = false;
            QString reserved;
            reserved.append(QString().fromStdString(data.collector_reserved()));
            //insert 0 to left side of string
            reserved.insert(0, QString().fill(QChar().fromAscii('0'),
                                              BYTES(12) * 2 - reserved.size()));
             // create Byte Array from string with HEX
            for (int i = 0; i < reserved.size(); i += 2)
                ba.append(((QString)reserved[i] +
                           (QString)reserved[i + 1]).toUInt(&isOk, BASE_HEX));
            switch (attrib)
            {
                case FieldName:
                    return QString("Reserved");
                case FieldValue:
                    return ba;
                case FieldTextValue:
                    return QString("0x%1").arg(reserved).toLower();
                case FieldFrameValue:
                    return ba;
                default:
                    break;
            }
            break;
        }
//--------------------------------------------terminator------------------------
        case lacp_tlv_type_terminator:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Terminator Information");
                case FieldValue:
                    return data.tlv_type_terminator();
                case FieldTextValue:
                    return QString("%1").arg(data.tlv_type_terminator());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.tlv_type_terminator());
                default:
                    break;
            }
            break;
        }
        case lacp_terminator_length:
        {
            switch (attrib)
            {
                case FieldName:
                    return QString("Terminator Length");
                case FieldValue:
                    return data.terminator_length();
                case FieldTextValue:
                    return QString("%1").arg(data.terminator_length());
                case FieldFrameValue:
                    return QByteArray(1, (char)data.terminator_length());
                default:
                    break;
            }
            break;
        }
        case lacp_terminator_reserved:
            {
            QByteArray ba;
            bool isOk = false;
            QString reserved;
            reserved.append(
                        QString().fromStdString(data.terminator_reserved()));
            //insert 0 to left side of string
            reserved.insert(0, QString().fill(QChar().fromAscii('0'),
                                              BYTES(50) * 2 - reserved.size()));
            // create Byte Array from string with HEX
            for (int i = 0; i < reserved.size(); i += 2)
                ba.append(((QString)reserved[i] +
                           (QString)reserved[i + 1]).toUInt(&isOk, BASE_HEX));
            switch (attrib)
            {
                case FieldName:
                    return QString("Reserved");
                case FieldValue:
                    return ba;
                case FieldTextValue:
                    return QString("0x%1").arg(reserved).toLower();
                case FieldFrameValue:
                    return ba;
                default:
                    break;
            }
            break;
        }
//------------------meta fields-------------------------------------------------
        case is_override_subtype:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_subtype();
                default:
                    break;
            }
        }
        case is_override_version:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_version_number();
                default:
                    break;
            }
        }
        case is_override_tlv_actor :
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_tlv_type_actor();
                default:
                    break;
            }
        }
        case is_override_actor_info:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_actor_info_length();
                default:
                    break;
            }
        }
        case is_override_actor_reserved:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_actor_reserved();
                default:
                    break;
            }
        }
        case is_override_tlv_partner:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_tlv_type_partner();
                default:
                    break;
            }
        }
        case is_override_partner_info:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_partner_info_length();
                default:
                    break;
            }
        }
        case is_override_partner_reserved:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_partner_reserved();
                default:
                    break;
            }
        }
        case is_override_tlv_collector:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_tlv_type_collector();
                default:
                    break;
            }
        }
        case is_override_collector_info:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_collector_info_length();
                default:
                    break;
            }
        }
        case is_override_collector_reserved:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_collector_reserved();
                default:
                    break;
            }
        }
        case is_override_tlv_terminator:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_tlv_type_terminator();
                default:
                    break;
            }
        }
        case is_override_terminator_info:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_terminator_length();
                default:
                    break;
            }
        }
        case is_override_terminator_reserved:
        {
            switch (attrib)
            {
                case FieldValue:
                    return data.is_override_terminator_reserved();
                default:
                    break;
            }
        }
        break;
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                   index);
            break;
    }
    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool LacpProtocol::setFieldData(int index, const QVariant &value,
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return isOk;

    switch (index)
    {
        case lacp_subtype:
        {
            uint subtype = value.toUInt(&isOk);
            if (isOk)
                data.set_subtype(subtype);
            break;
        }
        case lacp_version_number:
        {
            uint version_number = value.toUInt(&isOk);
            if (isOk)
                data.set_version_number(version_number);
            break;
        }
        case lacp_tlv_type_actor:
        {
            uint type_actor = value.toUInt(&isOk);
            if (isOk)
                data.set_tlv_type_actor(type_actor);
            break;
        }
        case lacp_actor_length:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_info_length(len);
            break;
        }
        case lacp_actor_system_priority:
        {
            uint priority = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_system_priority(priority);
            break;
        }
        case lacp_actor_system:
        {
            quint64 system = value.toString().toULongLong(&isOk, BASE_HEX);
            if (isOk)
                data.set_actor_system(system);
            break;
        }
        case lacp_actor_key:
        {
            uint key = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_key(key);
            break;
        }
        case lacp_actor_port_priority:
        {
            uint port_priority = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_port_priority(port_priority);
            break;
        }
        case lacp_actor_port:
        {
            uint port = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_port(port);
            break;
        }
        case lacp_actor_state:
        {
            uint state = value.toUInt(&isOk);
            if (isOk)
                data.set_actor_state(state);
            break;
        }
        case lacp_actor_reserved:
        {
            uint reserved = value.toString().toUInt(&isOk, BASE_HEX);
            if (isOk)
                data.set_actor_reserved(reserved);
            break;
        }
        // ----------------------------- partner -------------------------------
        case lacp_tlv_type_partner:
         {
             uint type_partner = value.toUInt(&isOk);
             if (isOk)
                 data.set_tlv_type_partner(type_partner);
             break;
         }
         case lacp_partner_length:
         {
             uint len = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_info_length(len);
             break;
         }
         case lacp_partner_system_priority:
         {
             uint priority = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_system_priority(priority);
             break;
         }
         case lacp_partner_system:
         {
             quint64 system = value.toString().toULongLong(&isOk, BASE_HEX);
             if (isOk)
                 data.set_partner_system(system);
             break;
         }
         case lacp_partner_key:
         {
             uint key = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_key(key);
             break;
         }
         case lacp_partner_port_priority:
         {
             uint port_priority = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_port_priority(port_priority);
             break;
         }
         case lacp_partner_port:
         {
             uint port = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_port(port);
             break;
         }
         case lacp_partner_state:
         {
             uint state = value.toUInt(&isOk);
             if (isOk)
                 data.set_partner_state(state);
             break;
         }
         case lacp_partner_reserved:
         {
             uint reserved = value.toString().toUInt(&isOk, BASE_HEX);
             if (isOk)
                 data.set_partner_reserved(reserved);
             break;
         }
        // ----------------------------- collector -----------------------------
        case lacp_tlv_type_collector:
         {
             uint type_collector = value.toUInt(&isOk);
             if (isOk)
                 data.set_tlv_type_collector(type_collector);
             break;
         }
         case lacp_collector_length:
         {
             uint len = value.toUInt(&isOk);
             if (isOk)
                 data.set_collector_info_length(len);
             break;
         }
        case lacp_collector_maxDelay:
        {
            uint state = value.toUInt(&isOk);
            if (isOk)
                data.set_collector_maxdelay(state);
            break;
        }
        case lacp_collector_reserved:
        {
            QByteArray reserved = value.toByteArray();
            data.set_collector_reserved(reserved.constData(), reserved.size());
            isOk = true;
            break;
        }
        // ----------------------------- terminator ----------------------------
        case lacp_tlv_type_terminator:
        {
            uint type_terminator = value.toUInt(&isOk);
            if (isOk)
                data.set_tlv_type_terminator(type_terminator);
            break;
         }
         case lacp_terminator_length:
         {
             uint len = value.toUInt(&isOk);
             if (isOk)
                 data.set_terminator_length(len);
             break;
        }
        case lacp_terminator_reserved:
        {
            QByteArray reserved = value.toByteArray();
            data.set_terminator_reserved(reserved.constData(), reserved.size());
            isOk = true;
            break;
        }
        // ----------------------------- meta ----------------------------------
        case is_override_subtype:
        {
            bool is_subt = value.toBool();
            data.set_is_override_subtype(is_subt);
            isOk = true;
            break;
        }
        case is_override_version:
        {
            bool is_ver = value.toBool();
            data.set_is_override_version_number(is_ver);
            isOk = true;
            break;
        }
        case is_override_tlv_actor:
        {
            bool is_act = value.toBool();
            data.set_is_override_tlv_type_actor(is_act);
            isOk = true;
            break;
        }
        case is_override_actor_info:
        {
            bool is_act_info = value.toBool();
            data.set_is_override_actor_info_length(is_act_info);
            isOk = true;
            break;
        }
        case is_override_actor_reserved:
        {
            bool is_act_res = value.toBool();
            data.set_is_override_actor_reserved(is_act_res);
            isOk = true;
            break;
        }
        case is_override_tlv_partner:
        {
            bool is_part = value.toBool();
            data.set_is_override_tlv_type_partner(is_part);
            isOk = true;
            break;
        }
        case is_override_partner_info:
        {
            bool is_part_info = value.toBool();
            data.set_is_override_partner_info_length(is_part_info);
            isOk = true;
            break;
        }
        case is_override_partner_reserved:
        {
            bool is_part_res = value.toBool();
            data.set_is_override_partner_reserved(is_part_res);
            isOk = true;
            break;
        }
        case is_override_tlv_collector:
        {
            bool is_coll = value.toBool();
            data.set_is_override_tlv_type_collector(is_coll);
            isOk = true;
            break;
        }
        case is_override_collector_info:
        {
            bool is_coll_info = value.toBool();
            data.set_is_override_collector_info_length(is_coll_info);
            isOk = true;
            break;
        }
        case is_override_collector_reserved:
        {
            bool is_coll_res = value.toBool();
            data.set_is_override_collector_reserved(is_coll_res);
            isOk = true;
            break;
        }
        case is_override_tlv_terminator:
        {
            bool is_term = value.toBool();
            data.set_is_override_tlv_type_terminator(is_term);
            isOk = true;
            break;
        }
        case is_override_terminator_info:
        {
            bool is_term_info = value.toBool();
            data.set_is_override_terminator_length(is_term_info);
            isOk = true;
            break;
        }
        case is_override_terminator_reserved:
        {
            bool is_term_res = value.toBool();
            data.set_is_override_terminator_reserved(is_term_res);
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
