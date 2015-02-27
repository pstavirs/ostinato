/*
Copyright (C) 2010 Srivats P.

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

#ifndef __INT_COMBO_BOX
#define __INT_COMBO_BOX

#include <QComboBox>

class IntComboBox : public QComboBox
{
public:
    IntComboBox(QWidget *parent = 0)
        : QComboBox(parent)
    {
        valueMask_ = 0xFFFFFFFF;
        setEditable(true);
    }
    void addItem(int value, const QString &text) 
    {
        QComboBox::addItem(
                QString("%1 - %2").arg(value & valueMask_).arg(text), 
                value);
    }
    int currentValue() 
    {
        bool isOk;
        int index = findText(currentText());
        if (index >= 0)
            return itemData(index).toInt();
        else
            return currentText().toInt(&isOk, 0);
    }
    void setValue(int value) 
    {
        int index = findData(value);
        if (index >= 0)
            setCurrentIndex(index);
        else
            setEditText(QString().setNum(value));
    }
    uint valueMask()
    {
        return valueMask_;
    }
    void setValueMask(uint mask)
    {
        valueMask_ = mask;
    }
private:
    uint valueMask_;
};

#endif
