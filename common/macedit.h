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

#ifndef _MAC_EDIT_H
#define _MAC_EDIT_H

#include <QLineEdit>
#include <QRegExpValidator>

class MacEdit: public QLineEdit
{
public:
    MacEdit(QWidget *parent = 0);

    quint64 value();
    void setValue(quint64 val);

protected:
    virtual void focusOutEvent(QFocusEvent *e);
};

inline MacEdit::MacEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Allow : or - as separator
    QRegExp reMac("([0-9,a-f,A-F]{0,2}[:-]){5,5}[0-9,a-f,A-F]{0,2}");

    setValidator(new QRegExpValidator(reMac, this));
}

inline quint64 MacEdit::value()
{
    QStringList bytes = text().split(QRegExp("[:-]"));
    quint64 mac = 0;

    while (bytes.count() > 6)
        bytes.removeLast();

    for (int i = 0; i < bytes.count(); i++)
        mac |= (bytes.at(i).toULongLong(NULL, 16) & 0xff) << (5-i)*8;

    return mac;
}

inline void MacEdit::setValue(quint64 val)
{
    setText(QString("%1").arg(val, 6*2, 16, QChar('0'))
        .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper());
}

inline void MacEdit::focusOutEvent(QFocusEvent *e)
{
    // be helpful and show a well-formatted value on focus out
    setValue(value());
    QLineEdit::focusOutEvent(e);
}

#endif

