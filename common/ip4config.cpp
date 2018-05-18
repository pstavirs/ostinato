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

#include "ip4config.h"
#include "ip4.h"

#include <QHostAddress>

Ip4ConfigForm::Ip4ConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    leIpVersion->setValidator(new QIntValidator(0, 15, this));
    leIpOptions->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]*"),
                                                   this));

    connect(cmbIpSrcAddrMode, SIGNAL(currentIndexChanged(int)),
        this, SLOT(on_cmbIpSrcAddrMode_currentIndexChanged(int)));
    connect(cmbIpDstAddrMode, SIGNAL(currentIndexChanged(int)),
        this, SLOT(on_cmbIpDstAddrMode_currentIndexChanged(int)));
}

Ip4ConfigForm::~Ip4ConfigForm()
{
}


Ip4ConfigForm* Ip4ConfigForm::createInstance()
{
    return new Ip4ConfigForm;
}

void Ip4ConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbIpVersionOverride->setChecked(
            proto->fieldData(
                Ip4Protocol::ip4_isOverrideVer,
                AbstractProtocol::FieldValue
            ).toBool());
    leIpVersion->setText(
            proto->fieldData(
                Ip4Protocol::ip4_ver, 
                AbstractProtocol::FieldValue
            ).toString());

    cbIpHdrLenOverride->setChecked(
            proto->fieldData(
                Ip4Protocol::ip4_isOverrideHdrLen,
                AbstractProtocol::FieldValue
            ).toBool());
    leIpHdrLen->setText(
            proto->fieldData(
                Ip4Protocol::ip4_hdrLen, 
                AbstractProtocol::FieldValue
            ).toString());

    tosDscp->setValue(
            proto->fieldData(
                Ip4Protocol::ip4_tos,
                AbstractProtocol::FieldValue
            ).toUInt());

    cbIpLengthOverride->setChecked(
            proto->fieldData(
                Ip4Protocol::ip4_isOverrideTotLen,
                AbstractProtocol::FieldValue
            ).toBool());
    leIpLength->setText(
            proto->fieldData(
                Ip4Protocol::ip4_totLen,
                AbstractProtocol::FieldValue
            ).toString());

    leIpId->setText(uintToHexStr(
            proto->fieldData(
                Ip4Protocol::ip4_id,
                AbstractProtocol::FieldValue
            ).toUInt(), 2));
    leIpFragOfs->setText(
            proto->fieldData(
                Ip4Protocol::ip4_fragOfs,
                AbstractProtocol::FieldValue
            ).toString());
    cbIpFlagsDf->setChecked((
            proto->fieldData(
                Ip4Protocol::ip4_flags,
                AbstractProtocol::FieldValue
            ).toUInt() & IP_FLAG_DF) > 0);
    cbIpFlagsMf->setChecked((
            proto->fieldData(
                Ip4Protocol::ip4_flags,
                AbstractProtocol::FieldValue
            ).toUInt() & IP_FLAG_MF) > 0);

    leIpTtl->setText(
            proto->fieldData(
                Ip4Protocol::ip4_ttl,
                AbstractProtocol::FieldValue
            ).toString());

    cbIpProtocolOverride->setChecked(
            proto->fieldData(
                Ip4Protocol::ip4_isOverrideProto,
                AbstractProtocol::FieldValue
            ).toBool());
    leIpProto->setText(uintToHexStr(
            proto->fieldData(
                Ip4Protocol::ip4_proto,
                AbstractProtocol::FieldValue
            ).toUInt(), 1));

    cbIpCksumOverride->setChecked(
            proto->fieldData(
                Ip4Protocol::ip4_isOverrideCksum,
                AbstractProtocol::FieldValue
            ).toBool());
    leIpCksum->setText(uintToHexStr(
            proto->fieldData(
                Ip4Protocol::ip4_cksum,
                AbstractProtocol::FieldValue
            ).toUInt(), 2));

    leIpSrcAddr->setText(QHostAddress(
            proto->fieldData(
                Ip4Protocol::ip4_srcAddr,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
    cmbIpSrcAddrMode->setCurrentIndex(
            proto->fieldData(
                Ip4Protocol::ip4_srcAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leIpSrcAddrCount->setText(
            proto->fieldData(
                Ip4Protocol::ip4_srcAddrCount,
                AbstractProtocol::FieldValue
            ).toString());
    leIpSrcAddrMask->setText(QHostAddress(
            proto->fieldData(
                Ip4Protocol::ip4_srcAddrMask,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());

    leIpDstAddr->setText(QHostAddress(
            proto->fieldData(
                Ip4Protocol::ip4_dstAddr,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
    cmbIpDstAddrMode->setCurrentIndex(
            proto->fieldData(
                Ip4Protocol::ip4_dstAddrMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leIpDstAddrCount->setText(
            proto->fieldData(
                Ip4Protocol::ip4_dstAddrCount,
                AbstractProtocol::FieldValue
            ).toString());
    leIpDstAddrMask->setText(QHostAddress(
            proto->fieldData(
                Ip4Protocol::ip4_dstAddrMask,
                AbstractProtocol::FieldValue
            ).toUInt()).toString());
    leIpOptions->setText(
            proto->fieldData(
                Ip4Protocol::ip4_options,
                AbstractProtocol::FieldValue
            ).toByteArray().toHex());
}

void Ip4ConfigForm::storeWidget(AbstractProtocol *proto)
{
    uint ff = 0;

    proto->setFieldData(
            Ip4Protocol::ip4_isOverrideVer,
            cbIpVersionOverride->isChecked());
    proto->setFieldData(
            Ip4Protocol::ip4_ver,
            leIpVersion->text());

    proto->setFieldData(
            Ip4Protocol::ip4_isOverrideHdrLen,
            cbIpHdrLenOverride->isChecked());
    proto->setFieldData(
            Ip4Protocol::ip4_hdrLen,
            leIpHdrLen->text());

    proto->setFieldData(
            Ip4Protocol::ip4_tos,
            tosDscp->value());

    proto->setFieldData(
            Ip4Protocol::ip4_totLen,
            leIpLength->text());
    proto->setFieldData(
            Ip4Protocol::ip4_isOverrideTotLen,
            cbIpLengthOverride->isChecked());

    proto->setFieldData(
            Ip4Protocol::ip4_id,
            hexStrToUInt(leIpId->text()));
    proto->setFieldData(
            Ip4Protocol::ip4_fragOfs,
            leIpFragOfs->text());

    if (cbIpFlagsDf->isChecked()) ff |= IP_FLAG_DF;
    if (cbIpFlagsMf->isChecked()) ff |= IP_FLAG_MF;
    proto->setFieldData(
            Ip4Protocol::ip4_flags,
            ff);

    proto->setFieldData(
            Ip4Protocol::ip4_ttl,
            leIpTtl->text());

    proto->setFieldData(
            Ip4Protocol::ip4_isOverrideProto,
            cbIpProtocolOverride->isChecked());
    proto->setFieldData(
            Ip4Protocol::ip4_proto,
            hexStrToUInt(leIpProto->text()));

    proto->setFieldData(
            Ip4Protocol::ip4_isOverrideCksum,
            cbIpCksumOverride->isChecked());
    proto->setFieldData(
            Ip4Protocol::ip4_cksum,
            hexStrToUInt(leIpCksum->text()));

    proto->setFieldData(
            Ip4Protocol::ip4_srcAddr,
            QHostAddress(leIpSrcAddr->text()).toIPv4Address());
    proto->setFieldData(
            Ip4Protocol::ip4_srcAddrMode,
            (OstProto::Ip4_IpAddrMode)cmbIpSrcAddrMode->currentIndex());
    proto->setFieldData(
            Ip4Protocol::ip4_srcAddrCount,
            leIpSrcAddrCount->text());
    proto->setFieldData(
            Ip4Protocol::ip4_srcAddrMask,
            QHostAddress(leIpSrcAddrMask->text()).toIPv4Address());

    proto->setFieldData(
            Ip4Protocol::ip4_dstAddr,
            QHostAddress(leIpDstAddr->text()).toIPv4Address());
    proto->setFieldData(
            Ip4Protocol::ip4_dstAddrMode,
            (OstProto::Ip4_IpAddrMode)cmbIpDstAddrMode->currentIndex());
    proto->setFieldData(
            Ip4Protocol::ip4_dstAddrCount,
            leIpDstAddrCount->text());
    proto->setFieldData(
            Ip4Protocol::ip4_dstAddrMask,
            QHostAddress(leIpDstAddrMask->text()).toIPv4Address());
    proto->setFieldData(
            Ip4Protocol::ip4_options,
            QByteArray::fromHex(QByteArray().append(leIpOptions->text())));
}

/*
 * Slots
 */
void Ip4ConfigForm::on_cmbIpSrcAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Ip4::e_im_fixed)
    {
        leIpSrcAddrCount->setDisabled(true);
        leIpSrcAddrMask->setDisabled(true);
    }
    else
    {
        leIpSrcAddrCount->setEnabled(true);
        leIpSrcAddrMask->setEnabled(true);
    }
}

void Ip4ConfigForm::on_cmbIpDstAddrMode_currentIndexChanged(int index)
{
    if (index == OstProto::Ip4::e_im_fixed)
    {
        leIpDstAddrCount->setDisabled(true);
        leIpDstAddrMask->setDisabled(true);
    }
    else
    {
        leIpDstAddrCount->setEnabled(true);
        leIpDstAddrMask->setEnabled(true);
    }
}
