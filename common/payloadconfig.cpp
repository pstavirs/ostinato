/*
Copyright (C) 2010-2014 Srivats P.

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

#include "payloadconfig.h"

#include "payload.h"

PayloadConfigForm::PayloadConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

PayloadConfigForm::~PayloadConfigForm()
{
}

AbstractProtocolConfigForm* PayloadConfigForm::createInstance()
{
    return new PayloadConfigForm;
}

void PayloadConfigForm::loadWidget(AbstractProtocol *proto)
{
    cmbPatternMode->setCurrentIndex(
            proto->fieldData(
                PayloadProtocol::payload_dataPatternMode,
                AbstractProtocol::FieldValue
                ).toUInt());
    lePattern->setText(uintToHexStr(
            proto->fieldData(
                PayloadProtocol::payload_dataPattern,
                AbstractProtocol::FieldValue
                ).toUInt(), 4));
}

void PayloadConfigForm::storeWidget(AbstractProtocol *proto)
{
    bool isOk;

    proto->setFieldData(
            PayloadProtocol::payload_dataPatternMode,
            cmbPatternMode->currentIndex());

    proto->setFieldData(
            PayloadProtocol::payload_dataPattern,
            lePattern->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));
}

void PayloadConfigForm::on_cmbPatternMode_currentIndexChanged(int index)
{
    switch(index)
    {
        case OstProto::Payload::e_dp_fixed_word:
            lePattern->setEnabled(true);
            break;
        case OstProto::Payload::e_dp_inc_byte:
        case OstProto::Payload::e_dp_dec_byte:
        case OstProto::Payload::e_dp_random:
            lePattern->setDisabled(true);
            break;
        default:
            qWarning("Unhandled/Unknown PatternMode = %d",index);
    }
}



