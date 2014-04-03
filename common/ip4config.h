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

#ifndef _IPV4_CONFIG_H
#define _IPV4_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_ip4.h"

class Ip4ConfigForm : 
    public AbstractProtocolConfigForm, 
    private Ui::ip4
{
    Q_OBJECT
public:
    Ip4ConfigForm(QWidget *parent = 0);
    virtual ~Ip4ConfigForm();

    static Ip4ConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);

private slots:
    void on_cmbIpSrcAddrMode_currentIndexChanged(int index);
    void on_cmbIpDstAddrMode_currentIndexChanged(int index);
};
#endif
