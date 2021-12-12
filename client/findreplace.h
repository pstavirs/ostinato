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

#ifndef _FIND_REPLACE_H
#define _FIND_REPLACE_H

#include "ui_findreplace.h"

class FindReplaceDialog: public QDialog, public Ui::FindReplace
{
    Q_OBJECT
public:
    struct Action;

    FindReplaceDialog(Action *action, QWidget *parent = 0);

private slots:
    void on_protocol_currentIndexChanged(const QString &name);
    void on_field_currentIndexChanged(int index);
    void on_matchAny_toggled(bool checked);
    void on_buttonBox_accepted();

private:
    struct FieldAttrib;

    quint32 protocolId_{0};
    Action *action_{nullptr};
    QList<FieldAttrib> fieldAttrib_;
};

struct FindReplaceDialog::Action
{
    QString protocolField;
    quint32 protocolNumber;
    quint32 fieldIndex;
    int fieldBitSize;
    QVariant findValue;
    QVariant findMask;
    QVariant replaceValue;
    QVariant replaceMask;

    bool selectedStreamsOnly; // in-out param
};

struct FindReplaceDialog::FieldAttrib
{
    quint32 index;
    int bitSize;
};
#endif

