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
along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _DEVICES_WIDGET_H
#define _DEVICES_WIDGET_H

#include "ui_deviceswidget.h"
#include <QWidget>

class PortGroupList;

class DevicesWidget: public QWidget, private Ui::DevicesWidget
{
    Q_OBJECT

public:
    DevicesWidget(QWidget *parent = NULL);
    void setPortGroupList(PortGroupList *portGroups);

    virtual void keyPressEvent(QKeyEvent *event);

public slots:
    void setCurrentPortIndex(const QModelIndex &portIndex);

private slots:
    void updateDeviceViewActions();

    void on_deviceInfo_toggled(bool checked);

    void on_actionNewDeviceGroup_triggered();
    void on_actionDeleteDeviceGroup_triggered();
    void on_actionEditDeviceGroup_triggered();
    void on_deviceGroupList_activated(const QModelIndex &index);

    void on_refresh_clicked();
    void on_resolveNeighbors_clicked();
    void on_clearNeighbors_clicked();

    void when_deviceList_currentChanged(const QModelIndex &index);

private:
    void setDeviceInfoButtonsVisible(bool show);

    PortGroupList *portGroups_;
    QModelIndex currentPortIndex_;
};

#endif

