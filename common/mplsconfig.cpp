/*
Copyright (C) 2010, 2014 Srivats P.
Copyright (C) 2015 Ilya Volchanetskiy

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

#include "mplsconfig.h"
#include "mpls.h"

MplsConfigForm::MplsConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

MplsConfigForm::~MplsConfigForm()
{
}

MplsConfigForm* MplsConfigForm::createInstance()
{
    return new MplsConfigForm;
}

void MplsConfigForm::loadWidget(AbstractProtocol *proto)
{
    mplsLabel->setText(
        proto->fieldData(
            MplsProtocol::mpls_label,
            AbstractProtocol::FieldValue
        ).toString());
    mplsExp->setText(
        proto->fieldData(
            MplsProtocol::mpls_exp,
            AbstractProtocol::FieldValue
        ).toString());
    mplsTtl->setText(
        proto->fieldData(
            MplsProtocol::mpls_ttl,
            AbstractProtocol::FieldValue
        ).toString());
    mplsBos->setText(
        proto->fieldData(
            MplsProtocol::mpls_bos,
            AbstractProtocol::FieldValue
        ).toString());
    mplsOverrideBos->setChecked(
        proto->fieldData(
            MplsProtocol::mpls_is_override_bos,
            AbstractProtocol::FieldValue
        ).toBool());
}

void MplsConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
        MplsProtocol::mpls_label,
        mplsLabel->text());
    proto->setFieldData(
        MplsProtocol::mpls_exp,
        mplsExp->text());
    proto->setFieldData(
        MplsProtocol::mpls_ttl,
        mplsTtl->text());
    proto->setFieldData(
        MplsProtocol::mpls_bos,
        mplsBos->text());
    proto->setFieldData(
        MplsProtocol::mpls_is_override_bos,
        mplsOverrideBos->isChecked());
}
