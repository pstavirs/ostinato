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

#include "arp.h"

#include <QHostAddress>
#include <QRegExp>

#define uintToMacStr(num)    \
    QString("%1").arg(num, 6*2, BASE_HEX, QChar('0')) \
        .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper()

ArpProtocol::ArpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    _hasPayload = false;
}

ArpProtocol::~ArpProtocol()
{
}

AbstractProtocol* ArpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new ArpProtocol(stream, parent);
}

quint32 ArpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kArpFieldNumber;
}

void ArpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::arp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void ArpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::arp))
        data.MergeFrom(protocol.GetExtension(OstProto::arp));
}

QString ArpProtocol::name() const
{
    return QString("Address Resolution Protocol");
}

QString ArpProtocol::shortName() const
{
    return QString("ARP");
}

/*!
  Return the ProtocolIdType for your protocol \n

  If your protocol doesn't have a protocolId field, you don't need to 
  reimplement this method - the base class implementation will do the 
  right thing
*/
#if 0
AbstractProtocol::ProtocolIdType ArpProtocol::protocolIdType() const
{
    return ProtocolIdIp;
}
#endif

/*!
  Return the protocolId for your protocol based on the 'type' requested \n

  If not all types are valid for your protocol, handle the valid type(s) 
  and for the remaining fallback to the base class implementation; if your 
  protocol doesn't have a protocolId at all, you don't need to reimplement
  this method - the base class will do the right thing
*/
quint32 ArpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdEth: return 0x0806;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int ArpProtocol::fieldCount() const
{
    return arp_fieldCount;
}

AbstractProtocol::FieldFlags ArpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case arp_hwType:
        case arp_protoType:

        case arp_hwAddrLen:
        case arp_protoAddrLen:

        case arp_opCode:

        case arp_senderHwAddr:
        case arp_senderProtoAddr:
        case arp_targetHwAddr:
        case arp_targetProtoAddr:
            break;

        case arp_senderHwAddrMode:
        case arp_senderHwAddrCount:

        case arp_senderProtoAddrMode:
        case arp_senderProtoAddrCount:
        case arp_senderProtoAddrMask:

        case arp_targetHwAddrMode:
        case arp_targetHwAddrCount:

        case arp_targetProtoAddrMode:
        case arp_targetProtoAddrCount:
        case arp_targetProtoAddrMask:
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

QVariant ArpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case arp_hwType:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Hardware Type");
                case FieldValue:
                    return data.hw_type();
                case FieldTextValue:
                    return QString("%1").arg(data.hw_type());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.hw_type(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case arp_protoType:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Protocol Type");
                case FieldValue:
                    return data.proto_type();
                case FieldTextValue:
                    return QString("%1").arg(data.proto_type(), 4, BASE_HEX, 
                            QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.proto_type(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case arp_hwAddrLen:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Hardware Address Length");
                case FieldValue:
                    return data.hw_addr_len();
                case FieldTextValue:
                    return QString("%1").arg(data.hw_addr_len());
                case FieldFrameValue:
                    return QByteArray(1, (char) data.hw_addr_len());
                default:
                    break;
            }
            break;
        }

        case arp_protoAddrLen:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Protocol Address Length");
                case FieldValue:
                    return data.proto_addr_len();
                case FieldTextValue:
                    return QString("%1").arg(data.proto_addr_len());
                case FieldFrameValue:
                    return QByteArray(1, (char) data.proto_addr_len());
                default:
                    break;
            }
            break;
        }

        case arp_opCode:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Operation Code");
                case FieldValue:
                    return data.op_code();
                case FieldTextValue:
                    return QString("%1").arg(data.op_code());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.op_code(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case arp_senderHwAddr:
        {
            int u;
            const int hwAddrStep = 1;
            quint64 hwAddr = 0;

            switch (data.sender_hw_addr_mode())
            {
                case OstProto::Arp::kFixed:
                    hwAddr = data.sender_hw_addr();
                    break;
                case OstProto::Arp::kIncrement:
                    u = (streamIndex % data.sender_hw_addr_count()) * 
                        hwAddrStep; 
                    hwAddr = data.sender_hw_addr() + u;
                    break;
                case OstProto::Arp::kDecrement:
                    u = (streamIndex % data.sender_hw_addr_count()) * 
                        hwAddrStep; 
                    hwAddr = data.sender_hw_addr() - u;
                    break;
                default:
                    qWarning("Unhandled hw_addr_mode %d", 
                            data.sender_hw_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Hardware Address");
                case FieldValue:
                    return hwAddr;
                case FieldTextValue:
                    return uintToMacStr(hwAddr);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian((quint64) hwAddr, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case arp_senderProtoAddr:
        {
            int        u;
            quint32    subnet, host, protoAddr = 0;

            switch(data.sender_proto_addr_mode())
            {
                case OstProto::Arp::kFixedHost:
                    protoAddr = data.sender_proto_addr();
                    break;
                case OstProto::Arp::kIncrementHost:
                    u = streamIndex % data.sender_proto_addr_count();
                    subnet = data.sender_proto_addr() 
                            & data.sender_proto_addr_mask();
                    host = (((data.sender_proto_addr() 
                                & ~data.sender_proto_addr_mask()) + u) 
                                    & ~data.sender_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                case OstProto::Arp::kDecrementHost:
                    u = streamIndex % data.sender_proto_addr_count();
                    subnet = data.sender_proto_addr() 
                            & data.sender_proto_addr_mask();
                    host = (((data.sender_proto_addr() 
                                & ~data.sender_proto_addr_mask()) - u) 
                                    & ~data.sender_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                case OstProto::Arp::kRandomHost:
                    subnet = data.sender_proto_addr() 
                            & data.sender_proto_addr_mask();
                    host = (qrand() & ~data.sender_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                default:
                    qWarning("Unhandled sender_proto_addr_mode = %d", 
                            data.sender_proto_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Protocol Address");
                case FieldValue:
                    return protoAddr;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) protoAddr, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QHostAddress(protoAddr).toString();
                default:
                    break;
            }
            break;
        }

        case arp_targetHwAddr:
        {
            int u;
            const int hwAddrStep = 1;
            quint64 hwAddr = 0;

            switch (data.target_hw_addr_mode())
            {
                case OstProto::Arp::kFixed:
                    hwAddr = data.target_hw_addr();
                    break;
                case OstProto::Arp::kIncrement:
                    u = (streamIndex % data.target_hw_addr_count()) * 
                        hwAddrStep; 
                    hwAddr = data.target_hw_addr() + u;
                    break;
                case OstProto::Arp::kDecrement:
                    u = (streamIndex % data.target_hw_addr_count()) * 
                        hwAddrStep; 
                    hwAddr = data.target_hw_addr() - u;
                    break;
                default:
                    qWarning("Unhandled hw_addr_mode %d", 
                            data.target_hw_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Hardware Address");
                case FieldValue:
                    return hwAddr;
                case FieldTextValue:
                    return uintToMacStr(hwAddr);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian((quint64) hwAddr, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        case arp_targetProtoAddr:
        {
            int        u;
            quint32    subnet, host, protoAddr = 0;

            switch(data.target_proto_addr_mode())
            {
                case OstProto::Arp::kFixed:
                    protoAddr = data.target_proto_addr();
                    break;
                case OstProto::Arp::kIncrementHost:
                    u = streamIndex % data.target_proto_addr_count();
                    subnet = data.target_proto_addr() 
                            & data.target_proto_addr_mask();
                    host = (((data.target_proto_addr() 
                                & ~data.target_proto_addr_mask()) + u) 
                                    & ~data.target_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                case OstProto::Arp::kDecrementHost:
                    u = streamIndex % data.target_proto_addr_count();
                    subnet = data.target_proto_addr() 
                            & data.target_proto_addr_mask();
                    host = (((data.target_proto_addr() 
                                & ~data.target_proto_addr_mask()) - u) 
                                    & ~data.target_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                case OstProto::Arp::kRandomHost:
                    subnet = data.target_proto_addr() 
                            & data.target_proto_addr_mask();
                    host = (qrand() & ~data.target_proto_addr_mask());
                    protoAddr = subnet | host;
                    break;
                default:
                    qWarning("Unhandled target_proto_addr_mode = %d", 
                            data.target_proto_addr_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Protocol Address");
                case FieldValue:
                    return protoAddr;
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) protoAddr, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QHostAddress(protoAddr).toString();
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case arp_senderHwAddrMode:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Hardware Address Mode");
                case FieldValue:
                    return data.sender_hw_addr_mode();
                default:
                    break;
            }
            break;
        case arp_senderHwAddrCount:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Hardware Address Count");
                case FieldValue:
                    return data.sender_hw_addr_count();
                default:
                    break;
            }
            break;
        case arp_senderProtoAddrMode:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Protocol Address Mode");
                case FieldValue:
                    return data.sender_proto_addr_mode();
                default:
                    break;
            }
            break;
        case arp_senderProtoAddrCount:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Protocol Address Count");
                case FieldValue:
                    return data.sender_proto_addr_count();
                default:
                    break;
            }
            break;
        case arp_senderProtoAddrMask:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sender Protocol Address Mask");
                case FieldValue:
                    return data.sender_proto_addr_mask();
                default:
                    break;
            }
            break;

        case arp_targetHwAddrMode:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Hardware Address Mode");
                case FieldValue:
                    return data.target_hw_addr_mode();
                default:
                    break;
            }
            break;
        case arp_targetHwAddrCount:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Hardware Address Count");
                case FieldValue:
                    return data.target_hw_addr_count();
                default:
                    break;
            }
            break;
        case arp_targetProtoAddrMode:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Protocol Address Mode");
                case FieldValue:
                    return data.target_proto_addr_mode();
                default:
                    break;
            }
            break;
        case arp_targetProtoAddrCount:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Protocol Address Count");
                case FieldValue:
                    return data.target_proto_addr_count();
                default:
                    break;
            }
            break;
        case arp_targetProtoAddrMask:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Target Protocol Address Mask");
                case FieldValue:
                    return data.target_proto_addr_mask();
                default:
                    break;
            }
            break;

       default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool ArpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case arp_hwType:
        {
            uint hwType = value.toUInt(&isOk);
            if (isOk)
                data.set_hw_type(hwType);
            break;
        }
        case arp_protoType:
        {
            uint protoType = value.toUInt(&isOk);
            if (isOk)
                data.set_proto_type(protoType);
            break;
        }
        case arp_hwAddrLen:
        {
            uint hwAddrLen = value.toUInt(&isOk);
            if (isOk)
                data.set_hw_addr_len(hwAddrLen);
            break;
        }
        case arp_protoAddrLen:
        {
            uint protoAddrLen = value.toUInt(&isOk);
            if (isOk)
                data.set_proto_addr_len(protoAddrLen);
            break;
        }
        case arp_opCode:
        {
            uint opCode = value.toUInt(&isOk);
            if (isOk)
                data.set_op_code(opCode);
            break;
        }

        case arp_senderHwAddr:
        {
            quint64 hwAddr = value.toULongLong(&isOk);
            if (isOk)
                data.set_sender_hw_addr(hwAddr);
            break;
        }
        case arp_senderHwAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.HwAddrMode_IsValid(mode))
                data.set_sender_hw_addr_mode((OstProto::Arp::HwAddrMode) mode);
            else
                isOk = false;
            break;
        }
        case arp_senderHwAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_sender_hw_addr_count(count);
            break;
        }

        case arp_senderProtoAddr:
        {
            uint protoAddr = value.toUInt(&isOk);
            if (isOk)
                data.set_sender_proto_addr(protoAddr);
            break;
        }
        case arp_senderProtoAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.ProtoAddrMode_IsValid(mode))
                data.set_sender_proto_addr_mode(
                        (OstProto::Arp::ProtoAddrMode)mode);
            else
                isOk = false;
            break;
        }
        case arp_senderProtoAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_sender_proto_addr_count(count);
            break;
        }
        case arp_senderProtoAddrMask:
        {
            uint mask = value.toUInt(&isOk);
            if (isOk)
                data.set_sender_proto_addr_mask(mask);
            break;
        }

        case arp_targetHwAddr:
        {
            quint64 hwAddr = value.toULongLong(&isOk);
            if (isOk)
                data.set_target_hw_addr(hwAddr);
            break;
        }
        case arp_targetHwAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.HwAddrMode_IsValid(mode))
                data.set_target_hw_addr_mode((OstProto::Arp::HwAddrMode)mode);
            else
                isOk = false;
            break;
        }
        case arp_targetHwAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_target_hw_addr_count(count);
            break;
        }

        case arp_targetProtoAddr:
        {
            uint protoAddr = value.toUInt(&isOk);
            if (isOk)
                data.set_target_proto_addr(protoAddr);
            break;
        }
        case arp_targetProtoAddrMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.ProtoAddrMode_IsValid(mode))
                data.set_target_proto_addr_mode(
                        (OstProto::Arp::ProtoAddrMode)mode);
            else
                isOk = false;
            break;
        }
        case arp_targetProtoAddrCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_target_proto_addr_count(count);
            break;
        }
        case arp_targetProtoAddrMask:
        {
            uint mask = value.toUInt(&isOk);
            if (isOk)
                data.set_target_proto_addr_mask(mask);
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

bool ArpProtocol::isProtocolFrameValueVariable() const
{
    if (fieldData(arp_senderHwAddrMode, FieldValue).toUInt() 
                != uint(OstProto::Arp::kFixed)
            || fieldData(arp_senderProtoAddrMode, FieldValue).toUInt() 
                != uint(OstProto::Arp::kFixed)
            || fieldData(arp_targetHwAddrMode, FieldValue).toUInt() 
                != uint(OstProto::Arp::kFixed)
            || fieldData(arp_targetProtoAddrMode, FieldValue).toUInt() 
                != uint(OstProto::Arp::kFixed))
        return true;

    return false;
}

int ArpProtocol::protocolFrameVariableCount() const
{
    int count = 1;

    if (fieldData(arp_senderHwAddrMode, FieldValue).toUInt() 
            != uint(OstProto::Arp::kFixed))
    {
        count = AbstractProtocol::lcm(count,
                fieldData(arp_senderHwAddrCount, FieldValue).toUInt());
    }

    if (fieldData(arp_senderProtoAddrMode, FieldValue).toUInt() 
            != uint(OstProto::Arp::kFixed))
    {
        count = AbstractProtocol::lcm(count,
                fieldData(arp_senderProtoAddrCount, FieldValue).toUInt());
    }

    if (fieldData(arp_targetHwAddrMode, FieldValue).toUInt() 
            != uint(OstProto::Arp::kFixed))
    {
        count = AbstractProtocol::lcm(count,
                fieldData(arp_targetHwAddrCount, FieldValue).toUInt());
    }

    if (fieldData(arp_targetProtoAddrMode, FieldValue).toUInt() 
            != uint(OstProto::Arp::kFixed))
    {
        count = AbstractProtocol::lcm(count,
                fieldData(arp_targetProtoAddrCount, FieldValue).toUInt());
    }

    return count;
}
