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

#include "udpconfig.h"
#include "udp.h"

UdpConfigForm::UdpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

UdpConfigForm::~UdpConfigForm()
{
}

UdpConfigForm* UdpConfigForm::createInstance()
{
    return new UdpConfigForm;
}


void UdpConfigForm::loadWidget(AbstractProtocol *proto)
{
    leUdpSrcPort->setText(
        proto->fieldData(
            UdpProtocol::udp_srcPort, 
            AbstractProtocol::FieldValue
        ).toString());
    cbUdpSrcPortOverride->setChecked(
        proto->fieldData(
            UdpProtocol::udp_isOverrideSrcPort, 
            AbstractProtocol::FieldValue
        ).toBool());
    leUdpDstPort->setText(
        proto->fieldData(
            UdpProtocol::udp_dstPort, 
            AbstractProtocol::FieldValue
        ).toString());
    cbUdpDstPortOverride->setChecked(
        proto->fieldData(
            UdpProtocol::udp_isOverrideDstPort, 
            AbstractProtocol::FieldValue
        ).toBool());

    leUdpLength->setText(
        proto->fieldData(
            UdpProtocol::udp_totLen, 
            AbstractProtocol::FieldValue
        ).toString());
    cbUdpLengthOverride->setChecked(
        proto->fieldData(
            UdpProtocol::udp_isOverrideTotLen, 
            AbstractProtocol::FieldValue
        ).toBool());

    leUdpCksum->setText(uintToHexStr(
        proto->fieldData(
            UdpProtocol::udp_cksum, 
            AbstractProtocol::FieldValue
        ).toUInt(), 2));
    cbUdpCksumOverride->setChecked(
        proto->fieldData(
            UdpProtocol::udp_isOverrideCksum, 
            AbstractProtocol::FieldValue
        ).toBool());
}

void UdpConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
        UdpProtocol::udp_srcPort,
        leUdpSrcPort->text());
    proto->setFieldData(
        UdpProtocol::udp_isOverrideSrcPort, 
        cbUdpSrcPortOverride->isChecked());
    proto->setFieldData(
        UdpProtocol::udp_dstPort,
        leUdpDstPort->text());
    proto->setFieldData(
        UdpProtocol::udp_isOverrideDstPort, 
        cbUdpDstPortOverride->isChecked());

    proto->setFieldData(
        UdpProtocol::udp_totLen,
        leUdpLength->text());
    proto->setFieldData(
        UdpProtocol::udp_isOverrideTotLen, 
        cbUdpLengthOverride->isChecked());

    proto->setFieldData(
        UdpProtocol::udp_cksum,
        hexStrToUInt(leUdpCksum->text()));
    proto->setFieldData(
        UdpProtocol::udp_isOverrideCksum, 
        cbUdpCksumOverride->isChecked());
}

