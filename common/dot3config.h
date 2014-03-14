/*
Copyright (C) 2010-2014 Srivats P.

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

#ifndef _DOT3_CONFIG_H
#define _DOT3_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_dot3.h"

class Dot3ConfigForm : 
    public AbstractProtocolConfigForm, 
    private Ui::dot3
{
    Q_OBJECT
public:
    Dot3ConfigForm(QWidget *parent = 0);
    virtual ~Dot3ConfigForm();

    static Dot3ConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);
};

#endif
