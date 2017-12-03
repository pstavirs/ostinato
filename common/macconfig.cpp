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

MacConfigForm::MacConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{

    setupUi(this);
    resolveInfo->hide();
#if 0
    // not working for some reason
    resolveInfo->setPixmap(resolveInfo->style()->standardIcon(
                               QStyle::SP_MessageBoxInformation).pixmap(128));
#endif
    leDstMacCount->setMinimum(1);
    leSrcMacCount->setMinimum(1);
    leDstMacStep->setMinimum(0);
    leSrcMacStep->setMinimum(0);
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
    switch (index) {
        case OstProto::Mac::e_mm_resolve:
            leDstMac->setEnabled(false);
            leDstMacCount->setEnabled(false);
            leDstMacStep->setEnabled(false);
            break;
        case OstProto::Mac::e_mm_fixed:
            leDstMac->setEnabled(true);
            leDstMacCount->setEnabled(false);
            leDstMacStep->setEnabled(false);
            break;
        default:
            leDstMac->setEnabled(true);
            leDstMacCount->setEnabled(true);
            leDstMacStep->setEnabled(true);
            break;
    }
    resolveInfo->setVisible(
            cmbDstMacMode->currentIndex() == OstProto::Mac::e_mm_resolve
            || cmbSrcMacMode->currentIndex() == OstProto::Mac::e_mm_resolve);
}

void MacConfigForm::on_cmbSrcMacMode_currentIndexChanged(int index)
{
    switch (index) {
        case OstProto::Mac::e_mm_resolve:
            leSrcMac->setEnabled(false);
            leSrcMacCount->setEnabled(false);
            leSrcMacStep->setEnabled(false);
            break;
        case OstProto::Mac::e_mm_fixed:
            leSrcMac->setEnabled(true);
            leSrcMacCount->setEnabled(false);
            leSrcMacStep->setEnabled(false);
            break;
        default:
            leSrcMac->setEnabled(true);
            leSrcMacCount->setEnabled(true);
            leSrcMacStep->setEnabled(true);
            break;
    }
    resolveInfo->setVisible(
            cmbDstMacMode->currentIndex() == OstProto::Mac::e_mm_resolve
            || cmbSrcMacMode->currentIndex() == OstProto::Mac::e_mm_resolve);
}

void MacConfigForm::loadWidget(AbstractProtocol *proto)
{
    leDstMac->setValue(
            proto->fieldData(
                MacProtocol::mac_dstAddr, 
                AbstractProtocol::FieldValue
            ).toULongLong());
    cmbDstMacMode->setCurrentIndex(
            proto->fieldData(
                MacProtocol::mac_dstMacMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leDstMacCount->setValue(
            proto->fieldData(
                MacProtocol::mac_dstMacCount,
                AbstractProtocol::FieldValue
            ).toUInt());
    leDstMacStep->setValue(
            proto->fieldData(
                MacProtocol::mac_dstMacStep,
                AbstractProtocol::FieldValue
            ).toUInt());

    leSrcMac->setValue(
            proto->fieldData(
                MacProtocol::mac_srcAddr, 
                AbstractProtocol::FieldValue
            ).toULongLong());
    cmbSrcMacMode->setCurrentIndex(
            proto->fieldData(
                MacProtocol::mac_srcMacMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    leSrcMacCount->setValue(
            proto->fieldData(
                MacProtocol::mac_srcMacCount,
                AbstractProtocol::FieldValue
            ).toUInt());
    leSrcMacStep->setValue(
            proto->fieldData(
                MacProtocol::mac_srcMacStep,
                AbstractProtocol::FieldValue
            ).toUInt());
}

void MacConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            MacProtocol::mac_dstAddr,
            leDstMac->value());
    proto->setFieldData(
            MacProtocol::mac_dstMacMode,
            cmbDstMacMode->currentIndex());
    proto->setFieldData(
            MacProtocol::mac_dstMacCount,
            leDstMacCount->value());
    proto->setFieldData(
            MacProtocol::mac_dstMacStep,
            leDstMacStep->value());

    proto->setFieldData(
            MacProtocol::mac_srcAddr,
            leSrcMac->value());
    proto->setFieldData(
            MacProtocol::mac_srcMacMode,
            cmbSrcMacMode->currentIndex());
    proto->setFieldData(
            MacProtocol::mac_srcMacCount,
            leSrcMacCount->value());
    proto->setFieldData(
            MacProtocol::mac_srcMacStep,
            leSrcMacStep->value());
}

