/*
Copyright (C) 2010-2012 Srivats P.

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

#ifndef _MAC_CONFIG_H
#define _MAC_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_mac.h"

class MacConfigForm : 
    public AbstractProtocolConfigForm,
    private Ui::mac
{
    Q_OBJECT
public:
    MacConfigForm(QWidget *parent = 0);
    virtual ~MacConfigForm();

    static MacConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);

private slots:
    void on_cmbDstMacMode_currentIndexChanged(int index);
    void on_cmbSrcMacMode_currentIndexChanged(int index);
};

#endif
