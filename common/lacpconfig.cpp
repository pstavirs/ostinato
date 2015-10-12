/*
Copyright (C) 2014 Marchuk S.

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

This module is developed by PLVision <developers@plvision.eu> company
*/

#include "lacpconfig.h"
#include "lacp.h"

#include <QRegExpValidator>
#include <QIntValidator>

#define ONE_BYTE_MAX 255
#define TWO_BYTE_MAX 65535
#define ONE_BIT(pos) ((unsigned int)(1 << (pos)))

LacpConfigForm::LacpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");
    setupUi(this);

    QRegExpValidator *validate_macAddress = new QRegExpValidator(reMac, this);
    QIntValidator *validate_oneByte_dec = 
            new QIntValidator(0, ONE_BYTE_MAX, this);
    QIntValidator *validate_twoByte_dec = 
            new QIntValidator(0, TWO_BYTE_MAX, this);

    ui_subtype->setValidator(validate_oneByte_dec);
    ui_version->setValidator(validate_oneByte_dec);

    ui_tlv_actor->setValidator(validate_oneByte_dec);
    ui_actor_length->setValidator(validate_oneByte_dec);
    ui_actor_system_priority->setValidator(validate_twoByte_dec);
    ui_actor_system->setValidator(validate_macAddress);
    ui_actor_key->setValidator(validate_twoByte_dec);
    ui_actor_port_priority->setValidator(validate_twoByte_dec);
    ui_actor_port->setValidator(validate_twoByte_dec);

    ui_tlv_partner->setValidator(validate_oneByte_dec);
    ui_partner_length->setValidator(validate_oneByte_dec);
    ui_partner_system_priority->setValidator(validate_twoByte_dec);
    ui_partner_system->setValidator(validate_macAddress);
    ui_partner_key->setValidator(validate_twoByte_dec);
    ui_partner_port_priority->setValidator(validate_twoByte_dec);
    ui_partner_port->setValidator(validate_twoByte_dec);

    ui_tlv_collector->setValidator(validate_oneByte_dec);
    ui_collector_length->setValidator(validate_oneByte_dec);
    ui_collector_MaxDelay->setValidator(validate_twoByte_dec);

    ui_tlv_terminator->setValidator(validate_oneByte_dec);
    ui_terminator_length->setValidator(validate_oneByte_dec);
}

LacpConfigForm::~LacpConfigForm()
{
}

LacpConfigForm* LacpConfigForm::createInstance()
{
    return new LacpConfigForm;
}

void LacpConfigForm::loadWidget(AbstractProtocol *proto)
{
    ui_subtype->setText(
                proto->fieldData(
                    LacpProtocol::lacp_subtype,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_version->setText(
                proto->fieldData(
                    LacpProtocol::lacp_version_number,
                    AbstractProtocol::FieldValue
                ).toString());
    //----------------------------actor-----------------------------------------
    ui_tlv_actor->setText(
                proto->fieldData(
                    LacpProtocol::lacp_tlv_type_actor,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_actor_length->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_length,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_actor_system_priority->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_system_priority,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_actor_system->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_system,
                    AbstractProtocol::FieldTextValue
                ).toString());
    ui_actor_key->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_key,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_actor_port_priority->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_port_priority,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_actor_port->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_port,
                    AbstractProtocol::FieldValue
                ).toString());

    quint8 actor_state = proto->fieldData(
                LacpProtocol::lacp_actor_state,
                AbstractProtocol::FieldValue
                ).toUInt();
    ui_actor_activity_check->setChecked(actor_state & ONE_BIT(0));
    ui_actor_timeout_check->setChecked(actor_state & ONE_BIT(1));
    ui_actor_aggregation_check->setChecked(actor_state & ONE_BIT(2));
    ui_actor_synchro_check->setChecked(actor_state & ONE_BIT(3));
    ui_actor_collecting_check->setChecked(actor_state & ONE_BIT(4));
    ui_actor_distributing_check->setChecked(actor_state & ONE_BIT(5));
    ui_actor_default_check->setChecked(actor_state & ONE_BIT(6));
    ui_actor_expiried_check->setChecked(actor_state & ONE_BIT(7));

    ui_actor_reserved->setText(
                proto->fieldData(
                    LacpProtocol::lacp_actor_reserved,
                    AbstractProtocol::FieldValue
                ).toString());
    //----------------------------partner---------------------------------------
    ui_tlv_partner->setText(
                proto->fieldData(
                    LacpProtocol::lacp_tlv_type_partner,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_partner_length->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_length,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_partner_system_priority->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_system_priority,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_partner_system->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_system,
                    AbstractProtocol::FieldTextValue
                ).toString());
    ui_partner_key->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_key,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_partner_port_priority->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_port_priority,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_partner_port->setText(
                proto->fieldData(
                    LacpProtocol::lacp_partner_port,
                    AbstractProtocol::FieldValue
                ).toString());

    quint8 partner_state = proto->fieldData(
                    LacpProtocol::lacp_partner_state,
                    AbstractProtocol::FieldValue
                ).toUInt();
    ui_partner_activity_check->setChecked(partner_state & ONE_BIT(0));
    ui_partner_timeout_check->setChecked(partner_state & ONE_BIT(1));
    ui_partner_aggregation_check->setChecked(partner_state & ONE_BIT(2));
    ui_partner_synchro_check->setChecked(partner_state & ONE_BIT(3));
    ui_partner_collecting_check->setChecked(partner_state & ONE_BIT(4));
    ui_partner_distributing_check->setChecked(partner_state & ONE_BIT(5));
    ui_partner_default_check->setChecked(partner_state & ONE_BIT(6));
    ui_partner_expiried_check->setChecked(partner_state & ONE_BIT(7));

    bool isOk;
    ui_partner_reserved->setText(QString("%1").arg(
                proto->fieldData(
                    LacpProtocol::lacp_partner_reserved,
                    AbstractProtocol::FieldValue
                ).toInt(&isOk), 6, BASE_HEX, QChar('0')));
    //---------------------------------collector--------------------------------
    ui_tlv_collector->setText(
                proto->fieldData(
                    LacpProtocol::lacp_tlv_type_collector,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_collector_length->setText(
                proto->fieldData(
                    LacpProtocol::lacp_collector_length,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_collector_MaxDelay->setText(
                proto->fieldData(
                    LacpProtocol::lacp_collector_maxDelay,
                    AbstractProtocol::FieldValue
                ).toString());
    QByteArray ba = proto->fieldData(
                LacpProtocol::lacp_collector_reserved,
                AbstractProtocol::FieldValue
                ).toByteArray();
    QString qs;
    for (int i = 0; i < ba.length(); i++)
        qs.append(QString("%1").arg((quint8)ba[i], 2, BASE_HEX, QChar('0')));
    ui_collector_reserved->setText(qs);
    //---------------------------------terminator-------------------------------
    ui_tlv_terminator->setText(
                proto->fieldData(
                    LacpProtocol::lacp_tlv_type_terminator,
                    AbstractProtocol::FieldValue
                ).toString());
    ui_terminator_length->setText(
                proto->fieldData(
                    LacpProtocol::lacp_terminator_length,
                    AbstractProtocol::FieldValue
                ).toString());
    ba = proto->fieldData(
                    LacpProtocol::lacp_terminator_reserved,
                    AbstractProtocol::FieldValue
                ).toByteArray();
    qs.clear();
    for (int i = 0; i < ba.length(); i++)
        qs.append(QString("%1").arg((quint8)ba[i], 2, BASE_HEX, QChar('0')));
    ui_terminator_reserved->setText(qs);
    //---------------------------------(meta fields) checkboxes-----------------
    ui_subtype_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_subtype,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_version_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_version,
                    AbstractProtocol::FieldValue
                ).toBool());
    //-------------------------------------actor--------------------------------
    ui_tlv_actor_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_tlv_actor,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_actor_length_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_actor_info,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_actor_reserved_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_actor_reserved,
                    AbstractProtocol::FieldValue
                ).toBool());
    //-------------------------------------partner------------------------------
    ui_tlv_partner_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_tlv_partner,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_partner_length_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_partner_info,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_partner_reserved_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_partner_reserved,
                    AbstractProtocol::FieldValue
                ).toBool());
    //-------------------------------------collector----------------------------
    ui_tlv_collector_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_tlv_collector,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_collector_length_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_collector_info,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_collector_reserved_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_collector_reserved,
                    AbstractProtocol::FieldValue
                ).toBool());
    //-------------------------------------terminator---------------------------
    ui_tlv_terminator_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_tlv_terminator,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_terminator_length_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_terminator_info,
                    AbstractProtocol::FieldValue
                ).toBool());
    ui_tlv_terminator_reserved_check->setChecked(
                proto->fieldData(
                    LacpProtocol::is_override_terminator_reserved,
                    AbstractProtocol::FieldValue
                ).toBool());
}

void LacpConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
                LacpProtocol::lacp_subtype,
                ui_subtype->text());
    proto->setFieldData(
                LacpProtocol::lacp_version_number,
                ui_version->text());
    //-------------------------------------actor--------------------------------
    proto->setFieldData(
                LacpProtocol::lacp_tlv_type_actor,
                ui_tlv_actor->text());
    proto->setFieldData(
                LacpProtocol::lacp_actor_length,
                ui_actor_length->text());
    proto->setFieldData(
                LacpProtocol::lacp_actor_system_priority,
                ui_actor_system_priority->text());
    proto->setFieldData(
                LacpProtocol::lacp_actor_system,
                ui_actor_system->text().remove(QChar(' ')));
    proto->setFieldData(
                LacpProtocol::lacp_actor_key,
                ui_actor_key->text());
    proto->setFieldData(
                LacpProtocol::lacp_actor_port_priority,
                ui_actor_port_priority->text());
    proto->setFieldData(
                LacpProtocol::lacp_actor_port,
                ui_actor_port->text());

    char actorState = 0;
    if (ui_actor_activity_check->isChecked())
        actorState = actorState | ONE_BIT(0);
    if (ui_actor_timeout_check->isChecked())
        actorState = actorState | ONE_BIT(1);
    if (ui_actor_aggregation_check->isChecked())
        actorState = actorState | ONE_BIT(2);
    if (ui_actor_synchro_check->isChecked())
        actorState = actorState | ONE_BIT(3);
    if (ui_actor_collecting_check->isChecked())
        actorState = actorState | ONE_BIT(4);
    if (ui_actor_distributing_check->isChecked())
        actorState = actorState | ONE_BIT(5);
    if (ui_actor_default_check->isChecked())
        actorState = actorState | ONE_BIT(6);
    if (ui_actor_expiried_check->isChecked())
        actorState = actorState | ONE_BIT(7);
    proto->setFieldData(
                LacpProtocol::lacp_actor_state,
                actorState);

    proto->setFieldData(
                LacpProtocol::lacp_actor_reserved,
                ui_actor_reserved->text().remove(QChar(' ')));
    //-------------------------------------partner------------------------------
    proto->setFieldData(
                LacpProtocol::lacp_tlv_type_partner,
                ui_tlv_partner->text());
    proto->setFieldData(
                LacpProtocol::lacp_partner_length,
                ui_partner_length->text());
    proto->setFieldData(
                LacpProtocol::lacp_partner_system_priority,
                ui_partner_system_priority->text());
    proto->setFieldData(
                LacpProtocol::lacp_partner_system,
                ui_partner_system->text().remove(QChar(' ')));
    proto->setFieldData(
                LacpProtocol::lacp_partner_key,
                ui_partner_key->text());
    proto->setFieldData(
                LacpProtocol::lacp_partner_port_priority,
                ui_partner_port_priority->text());
    proto->setFieldData(
                LacpProtocol::lacp_partner_port,
                ui_partner_port->text());

    char partnerState = 0;
    if (ui_partner_activity_check->isChecked())
        partnerState = partnerState | ONE_BIT(0);
    if (ui_partner_timeout_check->isChecked())
        partnerState = partnerState | ONE_BIT(1);
    if (ui_partner_aggregation_check->isChecked())
        partnerState = partnerState | ONE_BIT(2);
    if (ui_partner_synchro_check->isChecked())
        partnerState = partnerState | ONE_BIT(3);
    if (ui_partner_collecting_check->isChecked())
        partnerState = partnerState | ONE_BIT(4);
    if (ui_partner_distributing_check->isChecked())
        partnerState = partnerState | ONE_BIT(5);
    if (ui_partner_default_check->isChecked())
        partnerState = partnerState | ONE_BIT(6);
    if (ui_partner_expiried_check->isChecked())
        partnerState = partnerState | ONE_BIT(7);
    proto->setFieldData(
                LacpProtocol::lacp_partner_state,
                partnerState);
    proto->setFieldData(
                LacpProtocol::lacp_partner_reserved,
                ui_partner_reserved->text().remove(QChar(' ')));
    //-------------------------------------collector----------------------------
    proto->setFieldData(
                LacpProtocol::lacp_tlv_type_collector,
                ui_tlv_collector->text());
    proto->setFieldData(
                LacpProtocol::lacp_collector_length,
                ui_collector_length->text());
    proto->setFieldData(
                LacpProtocol::lacp_collector_maxDelay,
                ui_collector_MaxDelay->text());
    if (ui_collector_reserved->text().remove(QChar(' ')) != QString(""))
        proto->setFieldData(
                    LacpProtocol::lacp_collector_reserved,
                    ui_collector_reserved->text().remove(QChar(' ')));
    //-------------------------------------terminator---------------------------
    proto->setFieldData(
                LacpProtocol::lacp_tlv_type_terminator,
                ui_tlv_terminator->text());
    proto->setFieldData(
                LacpProtocol::lacp_terminator_length,
                ui_terminator_length->text());
    if (ui_terminator_reserved->text().remove(QChar(' ')) != QString(""))
        proto->setFieldData(
                    LacpProtocol::lacp_terminator_reserved,
                    ui_terminator_reserved->text().remove(QChar(' ')));

    //-------------------------------------checkboxes---------------------------
    proto->setFieldData(
                LacpProtocol::is_override_subtype,
                ui_subtype_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_version,
                ui_version_check->isChecked());
    //----------------actor--------------------
    proto->setFieldData(
                LacpProtocol::is_override_tlv_actor,
                ui_tlv_actor_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_actor_info,
                ui_tlv_actor_length_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_actor_reserved,
                ui_tlv_actor_reserved_check->isChecked());
    //----------------partner--------------------
    proto->setFieldData(
                LacpProtocol::is_override_tlv_partner,
                ui_tlv_partner_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_partner_info,
                ui_tlv_partner_length_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_partner_reserved,
                ui_tlv_partner_reserved_check->isChecked());
    //----------------collector--------------------
    proto->setFieldData(
                LacpProtocol::is_override_tlv_collector,
                ui_tlv_collector_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_collector_info,
                ui_tlv_collector_length_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_collector_reserved,
                ui_tlv_collector_reserved_check->isChecked());
    //----------------terminator--------------------
    proto->setFieldData(
                LacpProtocol::is_override_tlv_terminator,
                ui_tlv_terminator_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_terminator_info,
                ui_tlv_terminator_length_check->isChecked());
    proto->setFieldData(
                LacpProtocol::is_override_terminator_reserved,
                ui_tlv_terminator_reserved_check->isChecked());
}

