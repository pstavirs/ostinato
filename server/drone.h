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

#ifndef _DRONE_H
#define _DRONE_H

#include "ui_drone.h"

#include <QMenu>
#include <QSystemTrayIcon>

class RpcServer;
namespace OstProto { class OstService; }

class Drone : public QWidget, Ui::Drone 
{
     Q_OBJECT

public:
    Drone(QWidget *parent = 0);
    ~Drone();
    bool init();

signals:
    void hideMe(bool hidden);

protected:
    void changeEvent(QEvent *event);

private:
    QSystemTrayIcon            *trayIcon_;
    QMenu                    *trayIconMenu_;
    RpcServer               *rpcServer;
    OstProto::OstService    *service;

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

}; 
#endif
