/*
Copyright (C) 2021 Srivats P.

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

#include "fieldedit.h"

FieldEdit::FieldEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setType(kUInt64);
}

void FieldEdit::setType(FieldType type)
{
    // clear existing contents before changing the validator
    clear();
    setPlaceholderText("");

    type_ = type;
    switch (type_) {
        case kUInt64:
            setValidator(&uint64Validator_);
            if (isMaskMode_)
                setText("0xFFFFFFFFFFFFFFFF");
            break;
        case kMacAddress:
            setValidator(&macValidator_);
            setPlaceholderText("00:00:00:00:00:00");
            if (isMaskMode_)
                setText("FF:FF:FF:FF:FF:FF");
            break;
        case kIp4Address:
            setValidator(&ip4Validator_);
            setPlaceholderText("0.0.0.0");
            if (isMaskMode_)
                setText("255.255.255.255");
            break;
        case kIp6Address:
            setValidator(&ip6Validator_);
            setPlaceholderText("::");
            if (isMaskMode_)
                setText("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
            break;
        default:
            setValidator(nullptr);
            break;
    }
}

// Applicable only if type is kUInt64
void FieldEdit::setRange(quint64 min, quint64 max)
{
    uint64Validator_.setRange(min, max);
    if (type_ == kUInt64) {
        setPlaceholderText(QString("%1 - %2").arg(min).arg(max));
        if (isMaskMode_)
            setText(QString::number(max, 16).toUpper().prepend("0x"));
    }
}

void FieldEdit::setMaskMode(bool maskMode)
{
    isMaskMode_ = maskMode;
}

QString FieldEdit::text() const
{
    QString str = QLineEdit::text();

    switch (type_) {
        case kMacAddress:
            str.remove(QRegularExpression("[:-]"));
            str.prepend("0x");
            break;
        case kIp4Address:
            str = QString::number(QHostAddress(str).toIPv4Address());
            break;
        default:
            break;
    }

    return str;
}
