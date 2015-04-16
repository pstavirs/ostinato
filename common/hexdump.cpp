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

#include "hexdump.h"
#include "streambase.h"

HexDumpProtocol::HexDumpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

HexDumpProtocol::~HexDumpProtocol()
{
}

AbstractProtocol* HexDumpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new HexDumpProtocol(stream, parent);
}

quint32 HexDumpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kHexDumpFieldNumber;
}

void HexDumpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::hexDump)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void HexDumpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::hexDump))
        data.MergeFrom(protocol.GetExtension(OstProto::hexDump));
}

QString HexDumpProtocol::name() const
{
    return QString("HexDump");
}

QString HexDumpProtocol::shortName() const
{
    return QString("HexDump");
}

int HexDumpProtocol::fieldCount() const
{
    return hexDump_fieldCount;
}

AbstractProtocol::FieldFlags HexDumpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case hexDump_content:
            flags |= FrameField;
            break;

        case hexDump_pad_until_end:
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

QVariant HexDumpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case hexDump_content:
        {
            QByteArray ba;
            QByteArray pad;

            switch(attrib)
            {
                case FieldValue:
                case FieldTextValue:
                case FieldFrameValue:
                    ba.append(QString().fromStdString(data.content()));
                    if (data.pad_until_end())
                    {
                        pad = QByteArray(
                            protocolFrameSize(streamIndex) - ba.size(), '\0');
                    }
                    break;

                default:
                    break;
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Content");
                case FieldValue:
                    return ba;
                case FieldTextValue:
                    return ba.append(pad).toHex();
                case FieldFrameValue:
                    return ba.append(pad);
                default:
                    break;
            }
            break;

        }

        // Meta fields
        case hexDump_pad_until_end:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.pad_until_end();
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

bool HexDumpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case hexDump_content:
        {
            QByteArray ba = value.toByteArray();
            data.set_content(ba.constData(), ba.size());
            isOk = true;
            break;
        }
        case hexDump_pad_until_end:
        {
            bool pad = value.toBool();
            data.set_pad_until_end(pad);
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

int HexDumpProtocol::protocolFrameSize(int streamIndex) const
{
    int len = data.content().size();

    if (data.pad_until_end())
    {
        int pad = mpStream->frameLen(streamIndex) 
                    - (protocolFrameOffset(streamIndex) + len + kFcsSize);
        if (pad < 0)
            pad = 0;
        len += pad;
    }

    return len;
}

