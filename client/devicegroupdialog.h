/*
Copyright (C) 2016 Srivats P.

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

#ifndef _DEVICE_GROUP_DIALOG_H
#define _DEVICE_GROUP_DIALOG_H

#include "ui_devicegroupdialog.h"

#include <QDialog>

class Port;

class DeviceGroupDialog: public QDialog, private Ui::DeviceGroupDialog
{
    Q_OBJECT
public:
    DeviceGroupDialog(Port *port, int deviceGroupIndex,
            QWidget *parent = NULL, Qt::WindowFlags flags = 0);

    virtual void accept();
private slots:
    void on_vlanTagCount_valueChanged(int value);
    void on_vlans_cellChanged(int row, int col);
    void on_ipStack_currentIndexChanged(int index);

    void updateTotalVlanCount();
    void updateTotalDeviceCount();
    void updateIp4Gateway();
    void updateIp6Gateway();

    void loadDeviceGroup();
    void storeDeviceGroup();
private:
    static const int kMaxVlanTags = 4;

    Port *port_;
    int index_;
};

#endif
