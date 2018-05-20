/*
Copyright (C) 2018 Srivats P.

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

#ifndef _TOS_DSCP_H
#define _TOS_DSCP_H

#include "ui_tosdscp.h"
#include <QWidget>

class TosDscpWidget: public QWidget, private Ui::TosDscp
{
    Q_OBJECT

public:
    TosDscpWidget(QWidget *parent);

    int value();

public slots:
    void setValue(int value);

private slots:
    void setTosValue();
    void setDscpValue();

private:
    QMap<QString, quint8> codePoints_;
};

#endif

