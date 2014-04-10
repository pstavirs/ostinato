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

#include "mldconfig.h"
#include "mld.h"

#include "ipv6addressdelegate.h"
#include "ipv6addressvalidator.h"

MldConfigForm::MldConfigForm(QWidget *parent)
    : GmpConfigForm(parent)
{
    connect(msgTypeCombo, SIGNAL(currentIndexChanged(int)),
            SLOT(on_msgTypeCombo_currentIndexChanged(int)));

    msgTypeCombo->setValueMask(0xFF);
    msgTypeCombo->addItem(kMldV1Query,  "MLDv1 Query");
    msgTypeCombo->addItem(kMldV1Report, "MLDv1 Report");
    msgTypeCombo->addItem(kMldV1Done,   "MLDv1 Done");
    msgTypeCombo->addItem(kMldV2Query,  "MLDv2 Query");
    msgTypeCombo->addItem(kMldV2Report, "MLDv2 Report");

    _defaultGroupIp  = "::";
    _defaultSourceIp = "::";

    groupAddress->setValidator(new IPv6AddressValidator(this));
    groupRecordAddress->setValidator(new IPv6AddressValidator(this));
    sourceList->setItemDelegate(new IPv6AddressDelegate(this));
    groupRecordSourceList->setItemDelegate(new IPv6AddressDelegate(this));
}

MldConfigForm::~MldConfigForm()
{
}

MldConfigForm* MldConfigForm::createInstance()
{
    return new MldConfigForm;
}

void MldConfigForm::loadWidget(AbstractProtocol *proto)
{
    GmpConfigForm::loadWidget(proto);

    maxResponseTime->setText(
            proto->fieldData(
                MldProtocol::kMldMrt,
                AbstractProtocol::FieldValue
            ).toString());
}

void MldConfigForm::storeWidget(AbstractProtocol *proto)
{
    GmpConfigForm::storeWidget(proto);

    proto->setFieldData(
            MldProtocol::kMldMrt,
            maxResponseTime->text());
}

//
// -- private slots
//

void MldConfigForm::on_msgTypeCombo_currentIndexChanged(int /*index*/)
{
    switch(msgTypeCombo->currentValue())
    {
    case kMldV1Query:
    case kMldV1Report:
    case kMldV1Done:
        asmGroup->show();
        ssmWidget->hide();
        break;

    case kMldV2Query:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(kSsmQueryPage);
        ssmWidget->show();
        break;

    case kMldV2Report:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(kSsmReportPage);
        ssmWidget->show();
        break;

    default:
        asmGroup->hide();
        ssmWidget->hide();
        break;
    }
}
