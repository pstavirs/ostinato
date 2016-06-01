/*
Copyright (C) 2014 PLVision.

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

This module is developed by PLVision  <developers@plvision.eu>
*/

#include "stpconfig.h"
#include "stp.h"

#include <QRegExpValidator>
#include <QIntValidator>

#define ONE_BYTE_MAX 255
#define TWO_BYTE_MAX 65535
#define FOUR_BYTE_MAX 4294967295U
#define BIT_0 0
#define BIT_7 7
#define ONE_BIT(pos) ((unsigned int)(1 << (pos)))
#define BYTES(byte) (byte * sizeof(unsigned char))
#define STR_BYTES_LEN(len) (BYTES(len) * 2)

class UNumberValidator : public QValidator
{
private:
    quint64 min_;
    quint64 max_;

public:
    UNumberValidator(quint64 min, quint64 max, QObject * parent = 0)
        : QValidator(parent), min_(min), max_(max){}
    virtual ~UNumberValidator(){}

    virtual QValidator::State validate(QString& input, int& /*pos*/) const
    {
        QValidator::State state = QValidator::Acceptable;
        quint64 val = input.toULongLong();
        if(val < min_ || val > max_)
            state = QValidator::Invalid;
        return state;
    }
};


StpConfigForm::StpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");
    setupUi(this);

    QRegExpValidator *validateMACAddress =
        new QRegExpValidator(reMac, this);
    UNumberValidator *validateByte =
        new UNumberValidator(0, ONE_BYTE_MAX, this);
    UNumberValidator *validate2Byte =
        new UNumberValidator(0, TWO_BYTE_MAX, this);
    UNumberValidator *validate4Byte =
        new UNumberValidator(0, FOUR_BYTE_MAX, this);

    ui_protocol_id->setValidator(validate2Byte);
    ui_version_id->setValidator(validateByte);

    ui_bpdu_type->setValidator(validateByte);

    ui_root_id_priority->setValidator(validate2Byte);
    ui_root_id->setValidator(validateMACAddress);

    ui_root_path_cost->setValidator(validate4Byte);

    ui_bridge_id_priority->setValidator(validate2Byte);
    ui_bridge_id->setValidator(validateMACAddress);

    ui_port_id_priority->setValidator(validateByte);
    ui_port_id_number->setValidator(validateByte);

    ui_message_age->setValidator(validate2Byte);
    ui_max_age->setValidator(validate2Byte);
    ui_hello_time->setValidator(validate2Byte);
    ui_forward_delay->setValidator(validate2Byte);
}

StpConfigForm::~StpConfigForm()
{
}

StpConfigForm* StpConfigForm::createInstance()
{
    return new StpConfigForm;
}

void StpConfigForm::loadWidget(AbstractProtocol *proto)
{
    bool isOk;

    ui_protocol_id->setText(
        proto->fieldData(
            StpProtocol::stp_protocol_id,
            AbstractProtocol::FieldValue
        ).toString());
    ui_version_id->setText(
        proto->fieldData(
            StpProtocol::stp_version_id,
            AbstractProtocol::FieldValue
        ).toString());
    ui_bpdu_type->setText(
        proto->fieldData(
            StpProtocol::stp_bpdu_type,
            AbstractProtocol::FieldValue
        ).toString());

    quint8 flags = proto->fieldData(
            StpProtocol::stp_flags,
            AbstractProtocol::FieldValue
        ).toUInt();
    ui_flags_tc_check->setChecked(flags & ONE_BIT(BIT_0));
    ui_flags_tca_check->setChecked(flags & ONE_BIT(BIT_7));

    // root priority value stored as the first two bytes of stp_root_id
    // and the last 6 bytes are root MAC address (IEEE802.1D-2008)
    quint64 rootId = proto->fieldData(
        StpProtocol::stp_root_id,
        AbstractProtocol::FieldValue
        ).toULongLong(&isOk);

    ui_root_id->setText(
        QString::number(rootId & 0x0000FFFFFFFFFFFFULL, BASE_HEX));
    ui_root_id_priority->setText(QString::number(rootId >> 48));

    ui_root_path_cost->setText(
        proto->fieldData(
            StpProtocol::stp_root_path_cost,
            AbstractProtocol::FieldValue
        ).toString());

    // bridge priority value stored as the first two bytes of stp_bridge_id
    // and the last 6 bytes are bridge MAC address (IEEE802.1D-2008)
    quint64 bridgeId = proto->fieldData(
        StpProtocol::stp_bridge_id,
        AbstractProtocol::FieldValue
        ).toULongLong(&isOk);

    ui_bridge_id->setText(
        QString::number(bridgeId & 0x0000FFFFFFFFFFFFULL, BASE_HEX));
    ui_bridge_id_priority->setText(QString::number(bridgeId >> 48));

    // port priority is a first byte of stp_port_id field
    // and port ID is a second byte (IEEE802.1D-2008)
    uint portId = proto->fieldData(
        StpProtocol::stp_port_id,
        AbstractProtocol::FieldValue
        ).toUInt(&isOk);

    ui_port_id_priority->setText(QString::number(portId >> 8));
    ui_port_id_number->setText(QString::number(portId & ONE_BYTE_MAX));

    ui_message_age->setText(
        proto->fieldData(
            StpProtocol::stp_message_age,
            AbstractProtocol::FieldValue
        ).toString());
    ui_max_age->setText(
        proto->fieldData(
            StpProtocol::stp_max_age,
            AbstractProtocol::FieldValue
        ).toString());
    ui_hello_time->setText(
        proto->fieldData(
            StpProtocol::stp_hello_time,
            AbstractProtocol::FieldValue
        ).toString());
    ui_forward_delay->setText(
        proto->fieldData(
            StpProtocol::stp_forward_delay,
            AbstractProtocol::FieldValue
        ).toString());
}

void StpConfigForm::storeWidget(AbstractProtocol *proto)
{
    bool isOk;

    proto->setFieldData(
        StpProtocol::stp_protocol_id,
        QString("%1").arg(
            ui_protocol_id->text().toUInt(&isOk) & TWO_BYTE_MAX));

    proto->setFieldData(
        StpProtocol::stp_version_id,
        ui_version_id->text());
    proto->setFieldData(
        StpProtocol::stp_bpdu_type,
        ui_bpdu_type->text());

    char flags = 0;
    if (ui_flags_tc_check->isChecked()) flags = flags | ONE_BIT(BIT_0);
    if (ui_flags_tca_check->isChecked()) flags = flags | ONE_BIT(BIT_7);
    proto->setFieldData(StpProtocol::stp_flags, flags);

    // root priority value stored as the first two bytes of stp_root_id
    // and the last 6 bytes are root MAC address (IEEE802.1D-2008)
    quint64 rootIdPrio = ui_root_id_priority->text()
        .toULongLong(&isOk) & TWO_BYTE_MAX;
    quint64 rootId = hexStrToUInt64(
        ui_root_id->text()) | rootIdPrio << 48;
    proto->setFieldData(StpProtocol::stp_root_id, rootId);

    proto->setFieldData(
        StpProtocol::stp_root_path_cost,
        ui_root_path_cost->text());

    // bridge priority value stored as the first two bytes of stp_bridge_id
    // and the last 6 bytes are bridge MAC address (IEEE802.1D-2008)
    quint64 bridgeIdPrio =
        ui_bridge_id_priority->text().toULongLong(&isOk) & TWO_BYTE_MAX;
    quint64 bridgeId =
        hexStrToUInt64(ui_bridge_id->text()) | bridgeIdPrio << 48;
    proto->setFieldData(StpProtocol::stp_bridge_id, bridgeId);

    // port priority is a first byte of stp_port_id field
    // and port ID is a second byte (IEEE802.1D-2008)
    ushort portIdPrio =
        ui_port_id_priority->text().toUInt(&isOk, BASE_DEC) & ONE_BYTE_MAX;
    ushort portId =
        ui_port_id_number->text().toUInt(&isOk, BASE_DEC) & ONE_BYTE_MAX;
    proto->setFieldData(StpProtocol::stp_port_id, portIdPrio << 8 | portId);
    // timers
    proto->setFieldData(
        StpProtocol::stp_message_age,
        ui_message_age->text().toUInt(&isOk, BASE_DEC) & TWO_BYTE_MAX);
    proto->setFieldData(
        StpProtocol::stp_max_age,
        QString("%1").arg(
            ui_max_age->text().toUInt(&isOk, BASE_DEC) & TWO_BYTE_MAX));
    proto->setFieldData(
        StpProtocol::stp_hello_time,
        QString("%1").arg(
            ui_hello_time->text().toUInt(&isOk, BASE_DEC) & TWO_BYTE_MAX));
    proto->setFieldData(
        StpProtocol::stp_forward_delay,
        QString("%1").arg(
            ui_forward_delay->text().toUInt(&isOk, BASE_DEC) & TWO_BYTE_MAX));
}

