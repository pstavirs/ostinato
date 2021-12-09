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

#ifndef _FIELD_EDIT_H
#define _FIELD_EDIT_H

#include "ipv4addressvalidator.h"
#include "ipv6addressvalidator.h"
#include "macaddressvalidator.h"
#include "ulonglongvalidator.h"

#include <QLineEdit>

class FieldEdit: public QLineEdit
{
    Q_OBJECT
public:
    enum FieldType {
        kUInt64,
        kMacAddress,
        kIp4Address,
        kIp6Address
    };
    FieldEdit(QWidget *parent = 0);

    void setType(FieldType type);
    void setRange(quint64 min, quint64 max);

    void setMask(bool isMask); // FIXME: rename

    QString text() const;

private:
    FieldType type_{kUInt64};
    bool isMask_{false};

    IPv4AddressValidator ip4Validator_;
    IPv6AddressValidator ip6Validator_;
    MacAddressValidator macValidator_;
    ULongLongValidator uint64Validator_;
};

#endif

