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

#include "arpconfig.h"
#include "arp.h"

#include <QHostAddress>

ArpConfigForm::ArpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    opCodeCombo->setValidator(new QIntValidator(0, 0xFFFF, this));
    opCodeCombo->addItem(1, "ARP Request");
    opCodeCombo->addItem(2, "ARP Reply");

    connect(senderHwAddrMode, SIGNAL(currentIndexChanged(int)), 
            SLOT(on_senderHwAddrMode_currentIndexChanged(int)));
    connect(senderProtoAddrMode, SIGNAL(currentIndexChanged(int)), 
            SLOT(on_senderProtoAddrMode_currentIndexChanged(int)));
    connect(targetHwAddrMode, SIGNAL(currentIndexChanged(int)), 
            SLOT(on_targetHwAddrMode_currentIndexChanged(int)));
    connect(targetProtoAddrMode, SIGNAL(currentIndexChanged(int)), 
            SLOT(on_targetProtoAddrMode_currentIndexChanged(int)));
}

ArpConfigForm::~ArpConfigForm()
{
}

ArpConfigForm* ArpConfigForm::createInstance()
{
    return new ArpConfigForm;
}

void ArpConfigForm::loadWidget(AbstractProtocol *proto)
{
    hwType->setText(
            proto->fieldData(
                ArpProtocol::arp_hwType,
                AbstractProtocol::FieldValue
            ).toString());
    protoType->setText(uintToHexStr(
            proto->fieldData(
                ArpProtocol::arp_protoType,
                AbstractProtocol::FieldValue
            ).toUInt(), 2));
    hwAddrLen->setText(
            proto->fieldData(
                ArpProtocol::arp_hwAddrLen,
                AbstractProtocol::FieldValue
            ).toString());
    protoAddrLen->setText(
            proto->fieldData(
                ArpProtocol::arp_protoAddrLen,
                AbstractProtocol::FieldValue
            ).toString());

    opCodeCombo->setValue(
            proto->fieldData(
                ArpProtocol::arp_opCode,
                AbstractProtocol::FieldValue
            ).toUInt());

    senderHwAddr->setText(uintToHexStr(
            proto->fieldData(
                ArpProtocol::arp_senderHwAddr,
                AbstractProtocol::FieldValue
            ).toULongLong(), 6));
    senderHwAddrMode->setCurrentIndex(
            proto->fieldData(
                ArpProtocol::arp_senderHwAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    senderHwAddrCount->setText(
            proto->fieldData(
                ArpProtocol::arp_senderHwAddrCount,
                AbstractProtocol::FieldValue
            ).toString());

    senderProtoAddr->setText(QHostAddress(
            proto->fieldData(
                ArpProtocol::arp_senderProtoAddr,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
    senderProtoAddrMode->setCurrentIndex(
            proto->fieldData(
                ArpProtocol::arp_senderProtoAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    senderProtoAddrCount->setText(
            proto->fieldData(
                ArpProtocol::arp_senderProtoAddrCount,
                AbstractProtocol::FieldValue
            ).toString());
    senderProtoAddrMask->setText(QHostAddress(
            proto->fieldData(
                ArpProtocol::arp_senderProtoAddrMask,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());

    targetHwAddr->setText(uintToHexStr(
            proto->fieldData(
                ArpProtocol::arp_targetHwAddr,
                AbstractProtocol::FieldValue
            ).toULongLong(), 6));
    targetHwAddrMode->setCurrentIndex(
            proto->fieldData(
                ArpProtocol::arp_targetHwAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    targetHwAddrCount->setText(
            proto->fieldData(
                ArpProtocol::arp_targetHwAddrCount,
                AbstractProtocol::FieldValue
            ).toString());

    targetProtoAddr->setText(QHostAddress(
            proto->fieldData(
                ArpProtocol::arp_targetProtoAddr,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
    targetProtoAddrMode->setCurrentIndex(
            proto->fieldData(
                ArpProtocol::arp_targetProtoAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    targetProtoAddrCount->setText(
            proto->fieldData(
                ArpProtocol::arp_targetProtoAddrCount,
                AbstractProtocol::FieldValue
            ).toString());
    targetProtoAddrMask->setText(QHostAddress(
            proto->fieldData(
                ArpProtocol::arp_targetProtoAddrMask,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
}

void ArpConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            ArpProtocol::arp_hwType,
            hwType->text());
    proto->setFieldData(
            ArpProtocol::arp_protoType,
            hexStrToUInt(protoType->text()));
    proto->setFieldData(
            ArpProtocol::arp_hwAddrLen,
            hwAddrLen->text());
    proto->setFieldData(
            ArpProtocol::arp_protoAddrLen,
            protoAddrLen->text());

    proto->setFieldData(
            ArpProtocol::arp_opCode,
            opCodeCombo->currentValue());

    proto->setFieldData(
            ArpProtocol::arp_senderHwAddr,
            hexStrToUInt64(senderHwAddr->text()));
    proto->setFieldData(
            ArpProtocol::arp_senderHwAddrMode, 
            senderHwAddrMode->currentIndex());
    proto->setFieldData(
            ArpProtocol::arp_senderHwAddrCount,
            senderHwAddrCount->text());

    proto->setFieldData(
            ArpProtocol::arp_senderProtoAddr,
            QHostAddress(senderProtoAddr->text()).toIPv4Address());
    proto->setFieldData(
            ArpProtocol::arp_senderProtoAddrMode, 
            senderProtoAddrMode->currentIndex());
    proto->setFieldData(
            ArpProtocol::arp_senderProtoAddrCount, 
            senderProtoAddrCount->text());
    proto->setFieldData(
            ArpProtocol::arp_senderProtoAddrMask,
            QHostAddress(senderProtoAddrMask->text()).toIPv4Address());

    proto->setFieldData(
            ArpProtocol::arp_targetHwAddr,
            hexStrToUInt64(targetHwAddr->text()));
    proto->setFieldData(
            ArpProtocol::arp_targetHwAddrMode, 
            targetHwAddrMode->currentIndex());
    proto->setFieldData(
            ArpProtocol::arp_targetHwAddrCount,
            targetHwAddrCount->text());

    proto->setFieldData(
            ArpProtocol::arp_targetProtoAddr,
            QHostAddress(targetProtoAddr->text()).toIPv4Address());
    proto->setFieldData(
            ArpProtocol::arp_targetProtoAddrMode, 
            targetProtoAddrMode->currentIndex());
    proto->setFieldData(
            ArpProtocol::arp_targetProtoAddrCount, 
            targetProtoAddrCount->text());
    proto->setFieldData(
            ArpProtocol::arp_targetProtoAddrMask,
            QHostAddress(targetProtoAddrMask->text()).toIPv4Address());
}

/*
 * ------------ Private Slots --------------
 */

void ArpConfigForm::on_senderHwAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Arp::kFixed)
        senderHwAddrCount->setDisabled(true);
    else
        senderHwAddrCount->setEnabled(true);
}

void ArpConfigForm::on_targetHwAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Arp::kFixed)
        targetHwAddrCount->setDisabled(true);
    else
        targetHwAddrCount->setEnabled(true);
}

void ArpConfigForm::on_senderProtoAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Arp::kFixedHost)
    {
        senderProtoAddrCount->setDisabled(true);
        senderProtoAddrMask->setDisabled(true);
    }
    else
    {
        senderProtoAddrCount->setEnabled(true);
        senderProtoAddrMask->setEnabled(true);
    }
}

void ArpConfigForm::on_targetProtoAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Arp::kFixedHost)
    {
        targetProtoAddrCount->setDisabled(true);
        targetProtoAddrMask->setDisabled(true);
    }
    else
    {
        targetProtoAddrCount->setEnabled(true);
        targetProtoAddrMask->setEnabled(true);
    }
}

