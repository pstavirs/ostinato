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

#include "dot3config.h"
#include "dot3.h"
#include <QIntValidator>

Dot3ConfigForm::Dot3ConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
    leLength->setValidator(new QIntValidator(0, 16384, this));
}

Dot3ConfigForm::~Dot3ConfigForm()
{
}

Dot3ConfigForm* Dot3ConfigForm::createInstance()
{
    return new Dot3ConfigForm;
}

void Dot3ConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbOverrideLength->setChecked(
        proto->fieldData(
            Dot3Protocol::dot3_is_override_length,
            AbstractProtocol::FieldValue
        ).toBool());
    leLength->setText(
        proto->fieldData(
            Dot3Protocol::dot3_length,
            AbstractProtocol::FieldValue
        ).toString());
}

void Dot3ConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
        Dot3Protocol::dot3_is_override_length, 
        cbOverrideLength->isChecked());
    proto->setFieldData(
        Dot3Protocol::dot3_length,
        leLength->text());
}

