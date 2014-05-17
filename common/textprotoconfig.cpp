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

#include "textprotoconfig.h"
#include "textproto.h"

TextProtocolConfigForm::TextProtocolConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    portNumCombo->setValidator(new QIntValidator(0, 0xFFFF, this));
    portNumCombo->addItem(0, "Reserved");
    portNumCombo->addItem(80, "HTTP");
    portNumCombo->addItem(554, "RTSP");
    portNumCombo->addItem(5060, "SIP");
}

TextProtocolConfigForm::~TextProtocolConfigForm()
{
}

TextProtocolConfigForm* TextProtocolConfigForm::createInstance()
{
    return new TextProtocolConfigForm;
}

void TextProtocolConfigForm::loadWidget(AbstractProtocol *proto)
{
    portNumCombo->setValue(
            proto->fieldData(
                TextProtocol::textProto_portNum,
                AbstractProtocol::FieldValue
            ).toUInt());
    eolCombo->setCurrentIndex(
            proto->fieldData(
                TextProtocol::textProto_eol,
                AbstractProtocol::FieldValue
            ).toUInt());
    encodingCombo->setCurrentIndex(
            proto->fieldData(
                TextProtocol::textProto_encoding,
                AbstractProtocol::FieldValue
            ).toUInt());
    protoText->setText(
            proto->fieldData(
                TextProtocol::textProto_text,
                AbstractProtocol::FieldValue
            ).toString());
}

void TextProtocolConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            TextProtocol::textProto_portNum,
            portNumCombo->currentValue());
    proto->setFieldData(
            TextProtocol::textProto_eol,
            eolCombo->currentIndex());
    proto->setFieldData(
            TextProtocol::textProto_encoding,
            encodingCombo->currentIndex());

    proto->setFieldData(
            TextProtocol::textProto_text,
            protoText->toPlainText());
}

