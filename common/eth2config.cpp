/*
Copyright (C) 2010,2014 Srivats P.

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

#include "eth2config.h"
#include "eth2.h"

Eth2ConfigForm::Eth2ConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

Eth2ConfigForm::~Eth2ConfigForm()
{
}

Eth2ConfigForm* Eth2ConfigForm::createInstance()
{
    return new Eth2ConfigForm;
}

void Eth2ConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbOverrideType->setChecked(
        proto->fieldData(
            Eth2Protocol::eth2_is_override_type, 
            AbstractProtocol::FieldValue
        ).toBool());
    leType->setText(uintToHexStr(
        proto->fieldData(
            Eth2Protocol::eth2_type, 
            AbstractProtocol::FieldValue
        ).toUInt(), 2));
}

void Eth2ConfigForm::storeWidget(AbstractProtocol *proto)
{
    bool isOk;

    proto->setFieldData(
            Eth2Protocol::eth2_is_override_type, 
            cbOverrideType->isChecked());
    proto->setFieldData(
            Eth2Protocol::eth2_type, 
            leType->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));
}

