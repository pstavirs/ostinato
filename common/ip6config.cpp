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

#include "ip6config.h"
#include "ip6.h"
#include "ipv6addressvalidator.h"
#include <QHostAddress>

Ip6ConfigForm::Ip6ConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    version->setValidator(new QIntValidator(0, 0xF, this));
    payloadLength->setValidator(new QIntValidator(0, 0xFFFF, this));
    hopLimit->setValidator(new QIntValidator(0, 0xFF, this));

    srcAddr->setValidator(new IPv6AddressValidator(this));
    srcAddrCount->setValidator(new QIntValidator(this));
    //srcAddrPrefix->setValidator(new QIntValidator(0, 128, this));

    dstAddr->setValidator(new IPv6AddressValidator(this));
    dstAddrCount->setValidator(new QIntValidator(this));
    //dstAddrPrefix->setValidator(new QIntValidator(0, 128, this));
}

AbstractProtocolConfigForm* Ip6ConfigForm::createInstance()
{
    return new Ip6ConfigForm;
}

void Ip6ConfigForm::on_srcAddr_editingFinished()
{
    srcAddr->setText(QHostAddress(srcAddr->text()).toString());
}

void Ip6ConfigForm::on_dstAddr_editingFinished()
{
    dstAddr->setText(QHostAddress(dstAddr->text()).toString());
}

void Ip6ConfigForm::on_srcAddrModeCombo_currentIndexChanged(int index)
{
    bool enabled = (index > 0);

    srcAddrCount->setEnabled(enabled);
    srcAddrPrefix->setEnabled(enabled);
}

void Ip6ConfigForm::on_dstAddrModeCombo_currentIndexChanged(int index)
{
    bool enabled = (index > 0);

    dstAddrCount->setEnabled(enabled);
    dstAddrPrefix->setEnabled(enabled);
}

void Ip6ConfigForm::loadWidget(AbstractProtocol *ip6Proto)
{
    isVersionOverride->setChecked(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_isOverrideVersion,
            AbstractProtocol::FieldValue
            ).toBool());
    version->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_version, 
            AbstractProtocol::FieldValue
        ).toString());

    trafficClass->setText(uintToHexStr(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_trafficClass, 
            AbstractProtocol::FieldValue
        ).toUInt(), 1));

    flowLabel->setText(QString("%1").arg(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_flowLabel, 
            AbstractProtocol::FieldValue
        ).toUInt(), 5, BASE_HEX, QChar('0')));

    isPayloadLengthOverride->setChecked(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_isOverridePayloadLength, 
            AbstractProtocol::FieldValue
        ).toBool());
    payloadLength->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_payloadLength, 
            AbstractProtocol::FieldValue
        ).toString());

    isNextHeaderOverride->setChecked(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_isOverrideNextHeader, 
            AbstractProtocol::FieldValue
        ).toBool());
    nextHeader->setText(uintToHexStr(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_nextHeader, 
            AbstractProtocol::FieldValue
        ).toUInt(), 1));

    hopLimit->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_hopLimit, 
            AbstractProtocol::FieldValue
        ).toString());

    srcAddr->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_srcAddress, 
            AbstractProtocol::FieldTextValue
        ).toString());
    srcAddrModeCombo->setCurrentIndex(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_srcAddrMode, 
            AbstractProtocol::FieldValue
        ).toUInt());
    srcAddrCount->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_srcAddrCount, 
            AbstractProtocol::FieldValue
        ).toString());
    srcAddrPrefix->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_srcAddrPrefix, 
            AbstractProtocol::FieldValue
        ).toString());

    dstAddr->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_dstAddress, 
            AbstractProtocol::FieldTextValue
        ).toString());
    dstAddrModeCombo->setCurrentIndex(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_dstAddrMode, 
            AbstractProtocol::FieldValue
        ).toUInt());
    dstAddrCount->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_dstAddrCount, 
            AbstractProtocol::FieldValue
        ).toString());
    dstAddrPrefix->setText(
        ip6Proto->fieldData(
            Ip6Protocol::ip6_dstAddrPrefix,
            AbstractProtocol::FieldValue
        ).toString());
}

void Ip6ConfigForm::storeWidget(AbstractProtocol *ip6Proto)
{
    bool isOk;

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_isOverrideVersion, 
            isVersionOverride->isChecked());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_version,
            version->text());

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_trafficClass,
            trafficClass->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_flowLabel,
            flowLabel->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_isOverridePayloadLength, 
            isPayloadLengthOverride->isChecked());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_payloadLength,
            payloadLength->text());

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_isOverrideNextHeader, 
            isNextHeaderOverride->isChecked());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_nextHeader, 
            nextHeader->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_hopLimit,
            hopLimit->text());

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_srcAddress,
            srcAddr->text());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_srcAddrMode,
            srcAddrModeCombo->currentIndex());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_srcAddrCount,
            srcAddrCount->text());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_srcAddrPrefix,
            srcAddrPrefix->text());

    ip6Proto->setFieldData(
            Ip6Protocol::ip6_dstAddress,
            dstAddr->text());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_dstAddrMode,
            dstAddrModeCombo->currentIndex());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_dstAddrCount,
            dstAddrCount->text());
    ip6Proto->setFieldData(
            Ip6Protocol::ip6_dstAddrPrefix,
            dstAddrPrefix->text());
}

