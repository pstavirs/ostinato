/*
Copyright (C) 2010, 2014 Srivats P.

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

#include "sample.h"

SampleProtocol::SampleProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

SampleProtocol::~SampleProtocol()
{
}

AbstractProtocol* SampleProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SampleProtocol(stream, parent);
}

quint32 SampleProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSampleFieldNumber;
}

void SampleProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::sample)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SampleProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::sample))
        data.MergeFrom(protocol.GetExtension(OstProto::sample));
}

QString SampleProtocol::name() const
{
    return QString("Sample Protocol");
}

QString SampleProtocol::shortName() const
{
    return QString("SAMPLE");
}

/*!
  TODO Return the ProtocolIdType for your protocol \n

  If your protocol doesn't have a protocolId field, you don't need to 
  reimplement this method - the base class implementation will do the 
  right thing
*/
AbstractProtocol::ProtocolIdType SampleProtocol::protocolIdType() const
{
    return ProtocolIdIp;
}

/*!
  TODO Return the protocolId for your protoocol based on the 'type' requested \n

  If not all types are valid for your protocol, handle the valid type(s) 
  and for the remaining fallback to the base class implementation; if your 
  protocol doesn't have a protocolId at all, you don't need to reimplement
  this method - the base class will do the right thing
*/
quint32 SampleProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdLlc: return 0xFFFFFF;
        case ProtocolIdEth: return 0xFFFF;
        case ProtocolIdIp: return 0xFF;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int SampleProtocol::fieldCount() const
{
    return sample_fieldCount;
}

/*!
  TODO Return the number of frame fields for your protocol. A frame field
  is a field which has the FrameField flag set \n

  If your protocol has different sets of fields based on a OpCode/Type field
  (e.g. icmp), you MUST re-implement this function; however, if your protocol
  has a fixed set of frame fields always, you don't need to reimplement this 
  method - the base class implementation will do the right thing
*/
int SampleProtocol::frameFieldCount() const
{
    return AbstractProtocol::frameFieldCount();
}

/*!
  TODO Edit this function to return the appropriate flags for each field \n

  See AbstractProtocol::FieldFlags for more info
*/
AbstractProtocol::FieldFlags SampleProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case sample_a:
        case sample_b:
        case sample_payloadLength:
            break;

        case sample_checksum:
            flags |= CksumField;
            break;

        case sample_x:
        case sample_y:
            break;

        case sample_is_override_checksum:
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

/*!
TODO: Edit this function to return the data for each field

See AbstractProtocol::fieldData() for more info
*/
QVariant SampleProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case sample_a:
        {
            int a = data.ab() >> 13;

            switch(attrib)
            {
                case FieldName:            
                    return QString("A");
                case FieldValue:
                    return a;
                case FieldTextValue:
                    return QString("%1").arg(a);
                case FieldFrameValue:
                    return QByteArray(1, (char) a);
                case FieldBitSize:
                    return 3;
                default:
                    break;
            }
            break;

        }
        case sample_b:
        {
            int b = data.ab() & 0x1FFF;

            switch(attrib)
            {
                case FieldName:            
                    return QString("B");
                case FieldValue:
                    return b;
                case FieldTextValue:
                    return QString("%1").arg(b, 4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) b, (uchar*) fv.data()); 
                    return fv;
                }
                case FieldBitSize:
                    return 13;
                default:
                    break;
            }
            break;
        }

        case sample_payloadLength:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Payload Length");
                case FieldValue:
                    return protocolFramePayloadSize(streamIndex);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int totlen;
                    totlen = protocolFramePayloadSize(streamIndex);
                    fv.resize(2);
                    qToBigEndian((quint16) totlen, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return QString("%1").arg(
                        protocolFramePayloadSize(streamIndex));
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case sample_checksum:
        {
            quint16 cksum;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_checksum())
                        cksum = data.checksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumIp);
                    break;
                default:
                    cksum = 0; // avoid the 'maybe used unitialized' warning
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
                    return  QString("0x%1").arg(
                        cksum, 4, BASE_HEX, QChar('0'));;
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case sample_x:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("X");
                case FieldValue:
                    return data.x();
                case FieldTextValue:
                    // Use the following line for display in decimal
                    return QString("%1").arg(data.x());
                    // Use the following line for display in hexa-decimal
                    //return QString("%1").arg(data.x(), 8, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.x(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case sample_y:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Y");
                case FieldValue:
                    return data.y();
                case FieldTextValue:
                    // Use the following line for display in decimal
                    //return QString("%1").arg(data.y());
                    // Use the following line for display in hexa-decimal
                    return QString("%1").arg(data.y(), 4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.y(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }


        // Meta fields
        case sample_is_override_checksum:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_checksum();
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
bool SampleProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case sample_a:
        {
            uint a = value.toUInt(&isOk);
            if (isOk)
                data.set_ab((data.ab() & 0x1FFF) | ((a & 0x07) << 13));
            break;
        }
        case sample_b:
        {
            uint b = value.toUInt(&isOk);
            if (isOk)
                data.set_ab((data.ab() & 0xe000) | (b & 0x1FFF));
            break;
        }
        case sample_payloadLength:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_payload_length(len);
            break;
        }
        case sample_checksum:
        {
            uint csum = value.toUInt(&isOk);
            if (isOk)
                data.set_checksum(csum);
            break;
        }
        case sample_x:
        {
            uint x = value.toUInt(&isOk);
            if (isOk)
                data.set_x(x);
            break;
        }
        case sample_y:
        {
            uint y = value.toUInt(&isOk);
            if (isOk)
                data.set_y(y);
            break;
        }
        case sample_is_override_checksum:
        {
            bool ovr = value.toBool();
            data.set_is_override_checksum(ovr);
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

/*!
  TODO: Return the protocol frame size in bytes\n

  If your protocol has a fixed size - you don't need to reimplement this; the
  base class implementation is good enough
*/
int SampleProtocol::protocolFrameSize(int streamIndex) const
{
    return AbstractProtocol::protocolFrameSize(streamIndex);
}

/*!
  TODO: If your protocol frame size can vary across pkts of the same stream,
  return true \n

  Otherwise you don't need to reimplement this method - the base class always
  returns false
*/
bool SampleProtocol::isProtocolFrameSizeVariable() const
{
    return false;
}

/*!
  TODO: If your protocol frame has any variable fields or has a variable
  size, return the minimum number of frames required to vary the fields \n

  See AbstractProtocol::protocolFrameVariableCount() for more info
*/
int SampleProtocol::protocolFrameVariableCount() const
{
    return AbstractProtocol::protocolFrameVariableCount();
}
