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

#include "llcconfig.h"
#include "llc.h"

LlcConfigForm::LlcConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

LlcConfigForm::~LlcConfigForm()
{
}

LlcConfigForm* LlcConfigForm::createInstance()
{
    return new LlcConfigForm;
}

void LlcConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbOverrideDsap->setChecked(
            proto->fieldData(
                LlcProtocol::llc_is_override_dsap,
                AbstractProtocol::FieldValue
            ).toBool());
    leDsap->setText(uintToHexStr(
            proto->fieldData(
                LlcProtocol::llc_dsap,
                AbstractProtocol::FieldValue
            ).toUInt(), 1));

    cbOverrideSsap->setChecked(
            proto->fieldData(
                LlcProtocol::llc_is_override_ssap,
                AbstractProtocol::FieldValue
            ).toBool());
    leSsap->setText(uintToHexStr(
            proto->fieldData(
                LlcProtocol::llc_ssap,
                AbstractProtocol::FieldValue
            ).toUInt(), 1));

    cbOverrideControl->setChecked(
            proto->fieldData(
                LlcProtocol::llc_is_override_ctl,
                AbstractProtocol::FieldValue
            ).toBool());
    leControl->setText(uintToHexStr(
            proto->fieldData(
                LlcProtocol::llc_ctl,
                AbstractProtocol::FieldValue
            ).toUInt(), 1));
}

void 
LlcConfigForm::storeWidget(AbstractProtocol *proto)
{
    bool isOk;

    proto->setFieldData(
            LlcProtocol::llc_is_override_dsap, 
            cbOverrideDsap->isChecked());
    proto->setFieldData(
            LlcProtocol::llc_dsap,
            leDsap->text().toUInt(&isOk, BASE_HEX));

    proto->setFieldData(
            LlcProtocol::llc_is_override_ssap, 
            cbOverrideSsap->isChecked());
    proto->setFieldData(
            LlcProtocol::llc_ssap,
            leSsap->text().toUInt(&isOk, BASE_HEX));

    proto->setFieldData(
            LlcProtocol::llc_is_override_ctl, 
            cbOverrideControl->isChecked());
    proto->setFieldData(
            LlcProtocol::llc_ctl, 
            leControl->text().toUInt(&isOk, BASE_HEX));
}

