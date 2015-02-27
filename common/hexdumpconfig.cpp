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

#include "hexdumpconfig.h"
#include "hexdump.h"

HexDumpConfigForm::HexDumpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    hexEdit->setFont(QFont("Courier"));
    hexEdit->setOverwriteMode(false);
}

HexDumpConfigForm::~HexDumpConfigForm()
{
}

HexDumpConfigForm* HexDumpConfigForm::createInstance()
{
    return new HexDumpConfigForm;
}

void HexDumpConfigForm::loadWidget(AbstractProtocol *proto)
{
    hexEdit->setData(
            proto->fieldData(
                HexDumpProtocol::hexDump_content,
                AbstractProtocol::FieldValue
            ).toByteArray());
    padUntilEnd->setChecked(
            proto->fieldData(
                HexDumpProtocol::hexDump_pad_until_end,
                AbstractProtocol::FieldValue
            ).toBool());
}

void HexDumpConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            HexDumpProtocol::hexDump_content,
            hexEdit->data());
    proto->setFieldData(
            HexDumpProtocol::hexDump_pad_until_end,
            padUntilEnd->isChecked());
}

//
// ------------ private slots
//
void HexDumpConfigForm::on_hexEdit_overwriteModeChanged(bool isOverwriteMode)
{
    if (isOverwriteMode)
        mode->setText(tr("Ovr"));
    else
        mode->setText(tr("Ins"));
}

