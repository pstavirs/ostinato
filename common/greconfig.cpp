/*
Copyright (C) 2021 Srivats P.

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

#include "greconfig.h"
#include "gre.h"

GreConfigForm::GreConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

GreConfigForm::~GreConfigForm()
{
}

GreConfigForm* GreConfigForm::createInstance()
{
    return new GreConfigForm;
}

// Load widget contents from proto
void GreConfigForm::loadWidget(AbstractProtocol *proto)
{
    uint flags = proto->fieldData(GreProtocol::gre_flags,
                                 AbstractProtocol::FieldValue)
                             .toUInt();

    version->setValue(
        proto->fieldData(
            GreProtocol::gre_version,
            AbstractProtocol::FieldValue
        ).toUInt());

    hasChecksum->setChecked(flags & GRE_FLAG_CKSUM);
    checksum->setValue(
        proto->fieldData(
            GreProtocol::gre_checksum,
            AbstractProtocol::FieldValue
        ).toUInt());

    hasKey->setChecked(flags & GRE_FLAG_KEY);
    key->setValue(
        proto->fieldData(
            GreProtocol::gre_key,
            AbstractProtocol::FieldValue
        ).toUInt());

    hasSequence->setChecked(flags & GRE_FLAG_SEQ);
    sequence->setValue(
        proto->fieldData(
            GreProtocol::gre_sequence,
            AbstractProtocol::FieldValue
        ).toUInt());
}

// Store widget contents into proto
void GreConfigForm::storeWidget(AbstractProtocol *proto)
{
    uint flags = 0;

    if (hasChecksum->isChecked())
        flags |= GRE_FLAG_CKSUM;
    if (hasKey->isChecked())
        flags |= GRE_FLAG_KEY;
    if (hasSequence->isChecked())
        flags |= GRE_FLAG_SEQ;

    proto->setFieldData(
        GreProtocol::gre_flags,
        flags);

    proto->setFieldData(
        GreProtocol::gre_version,
        version->value());

    proto->setFieldData(
        GreProtocol::gre_checksum,
        checksum->value());

    proto->setFieldData(
        GreProtocol::gre_key,
        key->value());

    proto->setFieldData(
        GreProtocol::gre_sequence,
        sequence->value());
}

