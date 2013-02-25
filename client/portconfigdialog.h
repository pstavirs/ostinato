/*
Copyright (C) 2011 Srivats P.

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

#ifndef _PORT_CONFIG_DIALOG_H
#define _PORT_CONFIG_DIALOG_H

#include "ui_portconfigdialog.h"
#include "protocol.pb.h"
#include <QDialog>

class PortConfigDialog : public QDialog, public Ui::PortConfigDialog
{
public:
    PortConfigDialog(OstProto::Port &portConfig, QWidget *parent);

private:
    virtual void accept();

    OstProto::Port &portConfig_;
};

#endif

