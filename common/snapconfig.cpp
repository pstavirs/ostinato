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

#include "snapconfig.h"
#include "snap.h"

SnapConfigForm::SnapConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

SnapConfigForm::~SnapConfigForm()
{
}

SnapConfigForm* SnapConfigForm::createInstance()
{
    return new SnapConfigForm;
}

void SnapConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbOverrideOui->setChecked(
            proto->fieldData(
                SnapProtocol::snap_is_override_oui, 
                AbstractProtocol::FieldValue
            ).toBool());
    leOui->setText(uintToHexStr(
            proto->fieldData(
                SnapProtocol::snap_oui, 
                AbstractProtocol::FieldValue
            ).toUInt(), 3));

    cbOverrideType->setChecked(
            proto->fieldData(
                SnapProtocol::snap_is_override_type, 
                AbstractProtocol::FieldValue
            ).toBool());
    leType->setText(uintToHexStr(
            proto->fieldData(
                SnapProtocol::snap_type, 
                AbstractProtocol::FieldValue
            ).toUInt(), 2));
}

void SnapConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            SnapProtocol::snap_is_override_oui, 
            cbOverrideOui->isChecked());
    proto->setFieldData(
            SnapProtocol::snap_oui, 
            hexStrToUInt(leOui->text()));

    proto->setFieldData(
            SnapProtocol::snap_is_override_type, 
            cbOverrideType->isChecked());
    proto->setFieldData(
            SnapProtocol::snap_type, 
            hexStrToUInt(leType->text()));
}
