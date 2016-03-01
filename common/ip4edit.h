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

#ifndef _IP4_EDIT_H
#define _IP4_EDIT_H

#include <QHostAddress>
#include <QLineEdit>

class Ip4Edit: public QLineEdit
{
public:
    Ip4Edit(QWidget *parent = 0);

    quint32 value();
    void setValue(quint32 val);
};

inline Ip4Edit::Ip4Edit(QWidget *parent)
    : QLineEdit(parent)
{
    setInputMask(QString("000.000.000.000; "));
}

inline quint32 Ip4Edit::value()
{
    return QHostAddress(text()).toIPv4Address();
}

inline void Ip4Edit::setValue(quint32 val)
{
    setText(QHostAddress(val).toString());
}

#endif

