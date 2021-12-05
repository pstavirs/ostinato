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

    type_ = type;
    switch (type_) {
        case kUInt64:
            setValidator(&uint64Validator_);
            break;
        case kMacAddress:
            setValidator(&macValidator_);
            break;
        case kIp4Address:
            setValidator(&ip4Validator_);
            break;
        case kIp6Address:
            setValidator(&ip6Validator_);
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
}

QString FieldEdit::text() const
{
    QString str = QLineEdit::text();

    switch (type_) {
        case kMacAddress:
            str.remove(QRegularExpression("[:-]"));
            str.prepend("0x");
            break;
        default:
            break;
    }

    return str;
}
