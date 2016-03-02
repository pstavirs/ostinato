/*
Copyright (C) 2016 Srivats P.

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

#ifndef _IP6_EDIT_H
#define _IP6_EDIT_H

#include "ipv6addressvalidator.h"
#include "uint128.h"

#include <QHostAddress>
#include <QLineEdit>

class Ip6Edit: public QLineEdit
{
public:
    Ip6Edit(QWidget *parent = 0);

    UInt128 value();
    quint64 valueHi64();
    quint64 valueLo64();
    void setValue(UInt128 val);
    void setValue(quint64 hi, quint64 lo);
    void setValue(const QString &val);
};

inline Ip6Edit::Ip6Edit(QWidget *parent)
    : QLineEdit(parent)
{
    setValidator(new IPv6AddressValidator(this));
}

inline UInt128 Ip6Edit::value()
{
    Q_IPV6ADDR addr = QHostAddress(text()).toIPv6Address();
    return UInt128((quint8*)&addr);
}

inline quint64 Ip6Edit::valueHi64()
{
    return value().hi64();
}

inline quint64 Ip6Edit::valueLo64()
{
    return value().lo64();
}

inline void Ip6Edit::setValue(UInt128 val)
{
    setText(QHostAddress(val.toArray()).toString());
}

inline void Ip6Edit::setValue(quint64 hi, quint64 lo)
{
    UInt128 ip(hi, lo);
    setValue(ip);
}

inline void Ip6Edit::setValue(const QString &val)
{
    setText(QHostAddress(val).toString());
}
#endif
