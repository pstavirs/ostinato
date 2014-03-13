/*
Copyright (C) 2010,2013-2014 Srivats P.

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

#include "macconfig.h"
#include "mac.h"

#define MAX_MAC_ITER_COUNT  256

MacConfigForm::MacConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

    setupUi(this);
    leDstMac->setValidator(new QRegExpValidator(reMac, this));
    leSrcMac->setValidator(new QRegExpValidator(reMac, this));
    leDstMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
    leSrcMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
}

MacConfigForm::~MacConfigForm()
{
}

MacConfigForm* MacConfigForm::createInstance()
{
    MacConfigForm *f = new MacConfigForm;
    return f;
}

void MacConfigForm::on_cmbDstMacMode_currentIndexChanged(int index)
{
    if (index == OstProto::Mac::e_mm_fixed)
    {
        leDstMacCount->setEnabled(false);
        leDstMacStep->setEnabled(false);
    }
    else
    {
        leDstMacCount->setEnabled(true);
        leDstMacStep->setEnabled(true);
    }
}

void MacConfigForm::on_cmbSrcMacMode_currentIndexChanged(int index)
{
    if (index == OstProto::Mac::e_mm_fixed)
    {
        leSrcMacCount->setEnabled(false);
        leSrcMacStep->setEnabled(false);
    }
    else
    {
        leSrcMacCount->setEnabled(true);
        leSrcMacStep->setEnabled(true);
    }
}

void MacConfigForm::loadWidget(AbstractProtocol *proto)
{
    leDstMac->setText(
            proto->fieldData(
                MacProtocol::mac_dstAddr, 
                AbstractProtocol::FieldTextValue
            ).toString());
    cmbDstMacMode->setCurrentIndex(
            proto->fieldData(
                MacProtocol::mac_dstMacMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leDstMacCount->setText(
            proto->fieldData(
                MacProtocol::mac_dstMacCount,
                AbstractProtocol::FieldValue
            ).toString());
    leDstMacStep->setText(
            proto->fieldData(
                MacProtocol::mac_dstMacStep,
                AbstractProtocol::FieldValue
            ).toString());

    leSrcMac->setText(
            proto->fieldData(
                MacProtocol::mac_srcAddr, 
                AbstractProtocol::FieldTextValue
            ).toString());
    cmbSrcMacMode->setCurrentIndex(
            proto->fieldData(
                MacProtocol::mac_srcMacMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leSrcMacCount->setText(
            proto->fieldData(
                MacProtocol::mac_srcMacCount,
                AbstractProtocol::FieldValue
            ).toString());
    leSrcMacStep->setText(
            proto->fieldData(
                MacProtocol::mac_srcMacStep,
                AbstractProtocol::FieldValue
            ).toString());
}

void MacConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            MacProtocol::mac_dstAddr,
            leDstMac->text().remove(QChar(' ')));
    proto->setFieldData(
            MacProtocol::mac_dstMacMode,
            cmbDstMacMode->currentIndex());
    proto->setFieldData(
            MacProtocol::mac_dstMacCount,
            leDstMacCount->text());
    proto->setFieldData(
            MacProtocol::mac_dstMacStep,
            leDstMacStep->text());

    proto->setFieldData(
            MacProtocol::mac_srcAddr,
            leSrcMac->text().remove(QChar(' ')));
    proto->setFieldData(
            MacProtocol::mac_srcMacMode,
            cmbSrcMacMode->currentIndex());
    proto->setFieldData(
            MacProtocol::mac_srcMacCount,
            leSrcMacCount->text());
    proto->setFieldData(
            MacProtocol::mac_srcMacStep,
            leSrcMacStep->text());
}

