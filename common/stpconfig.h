/*
Copyright (C) 2014 PLVision.

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

This module is developed by PLVision  <developers@plvision.eu>
*/

#ifndef _STP_CONFIG_H
#define _STP_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_stp.h"

class StpConfigForm :
    public AbstractProtocolConfigForm,
    private Ui::Stp
{
    Q_OBJECT
public:
    StpConfigForm(QWidget *parent = 0);
    virtual ~StpConfigForm();

    static StpConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);
};

#endif
