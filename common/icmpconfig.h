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

#ifndef _ICMP_CONFIG_H
#define _ICMP_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_icmp.h"

class QButtonGroup;

class IcmpConfigForm : 
    public AbstractProtocolConfigForm,
    private Ui::Icmp
{
    Q_OBJECT
public:
    IcmpConfigForm(QWidget *parent = 0);
    virtual ~IcmpConfigForm();

    static IcmpConfigForm* createInstance();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);

private:
    QButtonGroup *versionGroup;

private slots:
    void on_typeCombo_currentIndexChanged(int index);
    void when_versionGroup_buttonClicked(int id);
};

#endif
