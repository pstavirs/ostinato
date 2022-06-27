/*
Copyright (C) 2022 Srivats P.

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

#ifndef _UINT_EDIT_H
#define _UINT_EDIT_H

#include "ulonglongvalidator.h"

#include <QLineEdit>

#include <limits.h>

class UIntEdit: public QLineEdit
{
public:
    UIntEdit(QWidget *parent = 0);

    quint32 value();
    void setValue(quint32 val);
};

// -------------------- //

inline UIntEdit::UIntEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setValidator(new ULongLongValidator(0, UINT_MAX));
}

inline quint32 UIntEdit::value()
{
    return text().toUInt(Q_NULLPTR, 0);
}

inline void UIntEdit::setValue(quint32 val)
{
    setText(QString::number(val));
}

#endif

