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
#ifndef _IP6_CONFIG_H
#define _IP6_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_ip6.h"

class Ip6ConfigForm : 
    public AbstractProtocolConfigForm, 
    private Ui::Ip6
{
    Q_OBJECT
public:
    Ip6ConfigForm(QWidget *parent = 0);
    static AbstractProtocolConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *ip6Proto);
    virtual void storeWidget(AbstractProtocol *ip6Proto);

private slots:
    void on_srcAddr_editingFinished();
    void on_dstAddr_editingFinished();
    void on_srcAddrModeCombo_currentIndexChanged(int index);
    void on_dstAddrModeCombo_currentIndexChanged(int index);
};

#endif
