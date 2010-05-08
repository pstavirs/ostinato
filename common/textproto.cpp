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

#include <qendian.h>

#include "textproto.h"

TextProtocolConfigForm::TextProtocolConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    portNumCombo->setValidator(new QIntValidator(0, 0xFFFF, this));
    portNumCombo->addItem(0, "Reserved");
    portNumCombo->addItem(80, "HTTP");
    portNumCombo->addItem(554, "RTSP");
    portNumCombo->addItem(5060, "SIP");
}

TextProtocol::TextProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    /* The configWidget is created lazily */
    configForm = NULL;
}

TextProtocol::~TextProtocol()
{
    delete configForm;
}

AbstractProtocol* TextProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new TextProtocol(stream, parent);
}

quint32 TextProtocol::protocolNumber() const
{
    return OstProto::Protocol::kTextProtocolFieldNumber;
}

void TextProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::textProtocol)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void TextProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::textProtocol))
        data.MergeFrom(protocol.GetExtension(OstProto::textProtocol));
}

QString TextProtocol::name() const
{
    return QString("Text Protocol");
}

QString TextProtocol::shortName() const
{
    return QString("TEXT");
}

quint32 TextProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdTcpUdp: return data.port_num();
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int TextProtocol::fieldCount() const
{
    return textProto_fieldCount;
}

AbstractProtocol::FieldFlags TextProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case textProto_text:
            break;

        case textProto_portNum:
        case textProto_encoding:
            flags |= FieldIsMeta;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant TextProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case textProto_text:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Text");
                case FieldValue:
                case FieldTextValue:
                    return QString().fromStdString(data.text());
                case FieldFrameValue:
                    Q_ASSERT(data.encoding() == OstProto::TextProtocol::kUtf8);
                    return QString().fromStdString(data.text()).toUtf8();
                default:
                    break;
            }
            break;

        }

        // Meta fields
        case textProto_portNum:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.port_num();
                default:
                    break;
            }
            break;
        }
        case textProto_encoding:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.encoding();
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

bool TextProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case textProto_text:
        {
            data.set_text(value.toString().toUtf8());
            isOk = true;
            break;
        }
        case textProto_portNum:
        {
            uint portNum = value.toUInt(&isOk);
            if (isOk)
                data.set_port_num(portNum);
            break;
        }
        case textProto_encoding:
        {
            uint enc = value.toUInt(&isOk);
            if (isOk && data.TextEncoding_IsValid(enc))
                data.set_encoding((OstProto::TextProtocol::TextEncoding) enc);
            else
                isOk = false;
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

int TextProtocol::protocolFrameSize(int streamIndex) const
{
    return fieldData(textProto_text, FieldFrameValue, streamIndex)
            .toByteArray().size() ;
}

QWidget* TextProtocol::configWidget()
{
    /* Lazy creation of the configWidget */
    if (configForm == NULL)
    {
        configForm = new TextProtocolConfigForm;
        loadConfigWidget();
    }

    return configForm;
}

void TextProtocol::loadConfigWidget()
{
    configWidget();

    configForm->portNumCombo->setValue(
            fieldData(textProto_portNum, FieldValue).toUInt());
    configForm->encodingCombo->setCurrentIndex(
            fieldData(textProto_encoding, FieldValue).toUInt());
    configForm->protoText->setText(
            fieldData(textProto_text, FieldValue).toString());
}

void TextProtocol::storeConfigWidget()
{
    configWidget();

    setFieldData(textProto_portNum, configForm->portNumCombo->currentValue());
    setFieldData(textProto_encoding, configForm->encodingCombo->currentIndex());

    setFieldData(textProto_text, configForm->protoText->toPlainText());
}

