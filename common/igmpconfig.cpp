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

#include "igmpconfig.h"
#include "igmp.h"
#include "ipv4addressdelegate.h"

IgmpConfigForm::IgmpConfigForm(QWidget *parent)
    : GmpConfigForm(parent)
{
    connect(msgTypeCombo, SIGNAL(currentIndexChanged(int)),
            SLOT(on_msgTypeCombo_currentIndexChanged(int)));

    msgTypeCombo->setValueMask(0xFF);
    msgTypeCombo->addItem(kIgmpV1Query,  "IGMPv1 Query");
    msgTypeCombo->addItem(kIgmpV1Report, "IGMPv1 Report");
    msgTypeCombo->addItem(kIgmpV2Query,  "IGMPv2 Query");
    msgTypeCombo->addItem(kIgmpV2Report, "IGMPv2 Report");
    msgTypeCombo->addItem(kIgmpV2Leave,  "IGMPv2 Leave");
    msgTypeCombo->addItem(kIgmpV3Query,  "IGMPv3 Query");
    msgTypeCombo->addItem(kIgmpV3Report, "IGMPv3 Report");

    _defaultGroupIp = "0.0.0.0";
    _defaultSourceIp = "0.0.0.0";

    groupAddress->setInputMask("009.009.009.009;"); // FIXME: use validator
    groupRecordAddress->setInputMask("009.009.009.009;"); // FIXME:use validator
    sourceList->setItemDelegate(new IPv4AddressDelegate(this));
    groupRecordSourceList->setItemDelegate(new IPv4AddressDelegate(this));
}

IgmpConfigForm::~IgmpConfigForm()
{
}

IgmpConfigForm* IgmpConfigForm::createInstance()
{
    return new IgmpConfigForm;
}

void IgmpConfigForm::loadWidget(AbstractProtocol *proto)
{
    GmpConfigForm::loadWidget(proto);

    maxResponseTime->setText(
            proto->fieldData(
                IgmpProtocol::kRsvdMrtCode,
                AbstractProtocol::FieldValue
            ).toString());
}

void IgmpConfigForm::storeWidget(AbstractProtocol *proto)
{
    GmpConfigForm::storeWidget(proto);

    proto->setFieldData(
            IgmpProtocol::kRsvdMrtCode,
            maxResponseTime->text());
}

//
// -- private slots
//

void IgmpConfigForm::on_msgTypeCombo_currentIndexChanged(int /*index*/)
{
    switch(msgTypeCombo->currentValue())
    {
    case kIgmpV1Query:
    case kIgmpV1Report:
    case kIgmpV2Query:
    case kIgmpV2Report:
    case kIgmpV2Leave:
        asmGroup->show();
        ssmWidget->hide();
        break;

    case kIgmpV3Query:
        asmGroup->show();
        ssmWidget->setCurrentIndex(kSsmQueryPage);
        ssmWidget->show();
        break;

    case kIgmpV3Report:
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

