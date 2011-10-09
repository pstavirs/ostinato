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

#include "dot3.h"
#include "streambase.h"

#include <QHostAddress>
#include <QIntValidator>
#include <qendian.h>

Dot3ConfigForm::Dot3ConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    leLength->setValidator(new QIntValidator(0, 16384, this));
}

Dot3Protocol::Dot3Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

Dot3Protocol::~Dot3Protocol()
{
    delete configForm;
}

AbstractProtocol* Dot3Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Dot3Protocol(stream, parent);
}

quint32 Dot3Protocol::protocolNumber() const
{
    return OstProto::Protocol::kDot3FieldNumber;
}

void Dot3Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::dot3)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Dot3Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::dot3))
        data.MergeFrom(protocol.GetExtension(OstProto::dot3));
}

QString Dot3Protocol::name() const
{
    return QString("802.3");
}

QString Dot3Protocol::shortName() const
{
    return QString("802.3");
}

int    Dot3Protocol::fieldCount() const
{
    return dot3_fieldCount;
}

AbstractProtocol::FieldFlags Dot3Protocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case dot3_length:
            break;

        case dot3_is_override_length:
            flags &= ~FrameField;
            flags |= MetaField;
            break;

        default:
            break;
    }

    return flags;
}

QVariant Dot3Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case dot3_length:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Length");
                case FieldValue:
                {
                    quint16 len = data.is_override_length() ?
                        data.length() : protocolFramePayloadSize(streamIndex);
                    return len;
                }
                case FieldTextValue:
                {
                    quint16 len = data.is_override_length() ?
                        data.length() : protocolFramePayloadSize(streamIndex);

                    return QString("%1").arg(len);
                }
                case FieldFrameValue:
                {
                    quint16 len = data.is_override_length() ?
                        data.length() : protocolFramePayloadSize(streamIndex);
                    QByteArray fv;

                    fv.resize(2);
                    qToBigEndian(len, (uchar*) fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;

        // Meta fields
        case dot3_is_override_length:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_length();
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

bool Dot3Protocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        return false;

    switch (index)
    {
        case dot3_length:
        {
            uint len = value.toUInt(&isOk);
            if (isOk)
                data.set_length(len);
            break;
        }
        case dot3_is_override_length:
        {
            bool ovr = value.toBool();
            data.set_is_override_length(ovr);
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

bool Dot3Protocol::isProtocolFrameValueVariable() const
{
    return isProtocolFramePayloadSizeVariable();
}

int Dot3Protocol::protocolFrameVariableCount() const
{
    return protocolFramePayloadVariableCount();
}

QWidget* Dot3Protocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new Dot3ConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void Dot3Protocol::loadConfigWidget()
{
    configWidget();

    configForm->cbOverrideLength->setChecked(
        fieldData(dot3_is_override_length, FieldValue).toBool());
    configForm->leLength->setText(
        fieldData(dot3_length, FieldValue).toString());
}

void Dot3Protocol::storeConfigWidget()
{
    configWidget();

    setFieldData(dot3_is_override_length, 
            configForm->cbOverrideLength->isChecked());
    setFieldData(dot3_length,configForm->leLength->text());
}

