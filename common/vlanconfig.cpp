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

#include "vlanconfig.h"
#include "vlan.h"

VlanConfigForm::VlanConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

VlanConfigForm::~VlanConfigForm()
{
}

VlanConfigForm* VlanConfigForm::createInstance()
{
    return new VlanConfigForm;
}

void VlanConfigForm::loadWidget(AbstractProtocol *proto)
{
    cbTpidOverride->setChecked(
            proto->fieldData(
                VlanProtocol::vlan_isOverrideTpid,
                AbstractProtocol::FieldValue
                ).toBool());
    leTpid->setText(uintToHexStr(
            proto->fieldData(
                VlanProtocol::vlan_tpid, 
                AbstractProtocol::FieldValue)
            .toUInt(), 2));
    cmbPrio->setCurrentIndex(
            proto->fieldData(
                VlanProtocol::vlan_prio,
                AbstractProtocol::FieldValue)
            .toUInt());
    cmbCfiDei->setCurrentIndex(
            proto->fieldData(
                VlanProtocol::vlan_cfiDei,
                AbstractProtocol::FieldValue)
            .toUInt());
    leVlanId->setText(
            proto->fieldData(
                VlanProtocol::vlan_vlanId,
                AbstractProtocol::FieldValue)
            .toString());
}

void VlanConfigForm::storeWidget(AbstractProtocol *proto)
{
    bool isOk;

    proto->setFieldData(
            VlanProtocol::vlan_isOverrideTpid,
            cbTpidOverride->isChecked());
    proto->setFieldData(
            VlanProtocol::vlan_tpid,
            leTpid->text().remove(QChar(' ')).toUInt(&isOk, BASE_HEX));
    proto->setFieldData(
            VlanProtocol::vlan_prio,
            cmbPrio->currentIndex());
    proto->setFieldData(
            VlanProtocol::vlan_cfiDei,
            cmbCfiDei->currentIndex());
    proto->setFieldData(
            VlanProtocol::vlan_vlanId,
            leVlanId->text());
}

