/*
Copyright (C) 2010, 2014 Srivats P.

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

#include "sampleconfig.h"
#include "sample.h"

SampleConfigForm::SampleConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

SampleConfigForm::~SampleConfigForm()
{
}

SampleConfigForm* SampleConfigForm::createInstance()
{
    return new SampleConfigForm;
}

/*!
TODO: Edit this function to load each field's data into the config Widget

See AbstractProtocolConfigForm::loadWidget() for more info
*/
void SampleConfigForm::loadWidget(AbstractProtocol *proto)
{
    sampleA->setText(
        proto->fieldData(
            SampleProtocol::sample_a,
            AbstractProtocol::FieldValue
        ).toString());
    sampleB->setText(
        proto->fieldData(
            SampleProtocol::sample_b,
            AbstractProtocol::FieldValue
        ).toString());

    samplePayloadLength->setText(
        proto->fieldData(
            SampleProtocol::sample_payloadLength,
            AbstractProtocol::FieldValue
        ).toString());

    isChecksumOverride->setChecked(
        proto->fieldData(
            SampleProtocol::sample_is_override_checksum,
            AbstractProtocol::FieldValue
        ).toBool());
    sampleChecksum->setText(uintToHexStr(
        proto->fieldData(
            SampleProtocol::sample_checksum,
            AbstractProtocol::FieldValue
        ).toUInt(), 2));

    sampleX->setText(
        proto->fieldData(
            SampleProtocol::sample_x,
            AbstractProtocol::FieldValue
        ).toString());
    sampleY->setText(
        proto->fieldData(
            SampleProtocol::sample_y,
            AbstractProtocol::FieldValue
        ).toString());
}

/*!
TODO: Edit this function to store each field's data from the config Widget

See AbstractProtocolConfigForm::storeWidget() for more info
*/
void SampleConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
        SampleProtocol::sample_a,
        sampleA->text());
    proto->setFieldData(
        SampleProtocol::sample_b,
        sampleB->text());

    proto->setFieldData(
        SampleProtocol::sample_payloadLength,
        samplePayloadLength->text());
    proto->setFieldData(
        SampleProtocol::sample_is_override_checksum, 
       
        isChecksumOverride->isChecked());
    proto->setFieldData(
        SampleProtocol::sample_checksum,
        hexStrToUInt(sampleChecksum->text()));

    proto->setFieldData(
        SampleProtocol::sample_x,
        sampleX->text());
    proto->setFieldData(
        SampleProtocol::sample_y,
        sampleY->text());
}

