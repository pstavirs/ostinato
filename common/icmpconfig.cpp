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

#include "icmpconfig.h"

#include "icmp.h"
#include "icmphelper.h"

#include <QButtonGroup>

IcmpConfigForm::IcmpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    versionGroup = new QButtonGroup(this);
    setupUi(this);

    // auto-connect's not working, for some reason I can't figure out!
    // slot name changed to when_ instead of on_ so that connectSlotsByName()
    // doesn't complain
    connect(versionGroup, 
            SIGNAL(buttonClicked(int)), 
            SLOT(when_versionGroup_buttonClicked(int)));

    versionGroup->addButton(icmp4Button, OstProto::Icmp::kIcmp4);
    versionGroup->addButton(icmp6Button, OstProto::Icmp::kIcmp6);

    typeCombo->setValidator(new QIntValidator(0, 0xFF, this));

    icmp4Button->click();

    idEdit->setValidator(new QIntValidator(0, 0xFFFF, this));
    seqEdit->setValidator(new QIntValidator(0, 0xFFFF, this));
}

IcmpConfigForm::~IcmpConfigForm()
{
}

IcmpConfigForm* IcmpConfigForm::createInstance()
{
    return new IcmpConfigForm;
}

void IcmpConfigForm::loadWidget(AbstractProtocol *proto)
{
    versionGroup->button(
            proto->fieldData(
                IcmpProtocol::icmp_version,
                AbstractProtocol::FieldValue
            ).toUInt())->click();

    typeCombo->setValue(
            proto->fieldData(
                IcmpProtocol::icmp_type,
                AbstractProtocol::FieldValue
            ).toUInt());
    codeEdit->setText(
            proto->fieldData(
                IcmpProtocol::icmp_code,
                AbstractProtocol::FieldValue
            ).toString());

    overrideCksum->setChecked(
            proto->fieldData(
                IcmpProtocol::icmp_is_override_checksum,
                AbstractProtocol::FieldValue
            ).toBool());
    cksumEdit->setText(uintToHexStr(
            proto->fieldData(
                IcmpProtocol::icmp_checksum,
                AbstractProtocol::FieldValue
            ).toUInt(), 2));

    idEdit->setText(
            proto->fieldData(
                IcmpProtocol::icmp_identifier,
                AbstractProtocol::FieldValue
            ).toString());
    seqEdit->setText(
            proto->fieldData(
                IcmpProtocol::icmp_sequence,
                AbstractProtocol::FieldValue
            ).toString());
}

void IcmpConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            IcmpProtocol::icmp_version,
            versionGroup->checkedId());

    proto->setFieldData(
            IcmpProtocol::icmp_type,
            typeCombo->currentValue());
    proto->setFieldData(
            IcmpProtocol::icmp_code,
            codeEdit->text());

    proto->setFieldData(
            IcmpProtocol::icmp_is_override_checksum, 
            overrideCksum->isChecked());
    proto->setFieldData(
            IcmpProtocol::icmp_checksum,
            hexStrToUInt(cksumEdit->text()));

    proto->setFieldData(
            IcmpProtocol::icmp_identifier,
            idEdit->text());
    proto->setFieldData(
            IcmpProtocol::icmp_sequence,
            seqEdit->text());
}

//
// -------- private slots
//
void IcmpConfigForm::on_typeCombo_currentIndexChanged(int /*index*/)
{
    idSeqFrame->setVisible(
        isIdSeqType(
            OstProto::Icmp::Version(versionGroup->checkedId()), 
            typeCombo->currentValue()));
}

void IcmpConfigForm::when_versionGroup_buttonClicked(int id)
{
    int value = typeCombo->currentValue();

    typeCombo->clear();

    switch(id)
    {
    case OstProto::Icmp::kIcmp4:
        typeCombo->addItem(kIcmpEchoReply, "Echo Reply");
        typeCombo->addItem(kIcmpDestinationUnreachable, 
                "Destination Unreachable");
        typeCombo->addItem(kIcmpSourceQuench, "Source Quench");
        typeCombo->addItem(kIcmpRedirect, "Redirect");
        typeCombo->addItem(kIcmpEchoRequest, "Echo Request");
        typeCombo->addItem(kIcmpTimeExceeded, "Time Exceeded");
        typeCombo->addItem(kIcmpParameterProblem, "Parameter Problem");
        typeCombo->addItem(kIcmpTimestampRequest, "Timestamp Request");
        typeCombo->addItem(kIcmpTimestampReply, "Timestamp Reply");
        typeCombo->addItem(kIcmpInformationRequest, "Information Request");
        typeCombo->addItem(kIcmpInformationReply, "Information Reply");
        typeCombo->addItem(kIcmpAddressMaskRequest, "Address Mask Request");
        typeCombo->addItem(kIcmpAddressMaskReply, "Address Mask Reply");
        break; 

    case OstProto::Icmp::kIcmp6:
        typeCombo->addItem(kIcmp6DestinationUnreachable, 
                "Destination Unreachable");
        typeCombo->addItem(kIcmp6PacketTooBig, "Packet Too Big");
        typeCombo->addItem(kIcmp6TimeExceeded, "Time Exceeded");
        typeCombo->addItem(kIcmp6ParameterProblem, "Parameter Problem");

        typeCombo->addItem(kIcmp6EchoRequest, "Echo Request");
        typeCombo->addItem(kIcmp6EchoReply, "Echo Reply");
        typeCombo->addItem(kIcmp6RouterSolicitation, "Router Solicitation");
        typeCombo->addItem(kIcmp6RouterAdvertisement, "Router Advertisement");
        typeCombo->addItem(kIcmp6NeighbourSolicitation, 
                "Neighbour Solicitation");
        typeCombo->addItem(kIcmp6NeighbourAdvertisement, 
                "Neighbour Advertisement");
        typeCombo->addItem(kIcmp6Redirect, "Redirect");
        typeCombo->addItem(kIcmp6InformationQuery, "Information Query");
        typeCombo->addItem(kIcmp6InformationResponse, "Information Response");
        break;
    default:
        Q_ASSERT(false);
    }

    typeCombo->setValue(value);
}

