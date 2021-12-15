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

#ifndef _MANDATORY_FIELDS_GROUP_H
#define _MANDATORY_FIELDS_GROUP_H

#include <QObject.h>
#include <QList.h>

class QPushButton;
class QWidget;

// Adapted from https://doc.qt.io/archives/qq/qq11-mandatoryfields.html
// and improved

class MandatoryFieldsGroup : public QObject
{
    Q_OBJECT

public:
    MandatoryFieldsGroup(QObject *parent)
        : QObject(parent)
    {
    }

    void add(QWidget *widget);
    void remove(QWidget *widget);
    void setSubmitButton(QPushButton *button);

public slots:
    void clear();

private slots:
    void changed();

private:
    QList<const QWidget*> widgets_;
    QPushButton *submitButton_{nullptr};
};


#endif

