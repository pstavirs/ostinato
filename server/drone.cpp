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

#include "drone.h"

#include "rpcserver.h"
#include "myservice.h"

#include <QCloseEvent>
#include <QMessageBox>

extern int myport;
extern const char* version;
extern const char* revision;

Drone::Drone(QWidget *parent)
     : QWidget(parent)
{
    setupUi(this);
    versionLabel->setText(
            QString("Version: %1 Revision: %2").arg(version).arg(revision));

    rpcServer = new RpcServer();
    service = new MyService();
}

Drone::~Drone()
{
    trayIcon_->hide();

    delete trayIcon_;
    delete trayIconMenu_;
    delete rpcServer;
    delete service;
}

bool Drone::init()
{
    Q_ASSERT(rpcServer);

    if (!rpcServer->registerService(service, myport ? myport : 7878))
    {
        QMessageBox::critical(0, qApp->applicationName(),
                rpcServer->errorString());
        return false;
    }

    trayIconMenu_ = new QMenu(this);

    trayIconMenu_->addAction(actionShow);
    trayIconMenu_->addAction(actionExit);
    trayIconMenu_->setDefaultAction(actionShow);
    trayIcon_ = new QSystemTrayIcon();
    trayIcon_->setIcon(QIcon(":/icons/portgroup.png"));
    trayIcon_->setToolTip(qApp->applicationName());
    trayIcon_->setContextMenu(trayIconMenu_);
    trayIcon_->show();

    connect(trayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    connect(this, SIGNAL(hideMe(bool)), this, SLOT(setHidden(bool)), 
            Qt::QueuedConnection);

    return true;
}

void Drone::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized())
    {
        emit hideMe(true);
        event->ignore();
        return;
    }

    QWidget::changeEvent(event);
}

void Drone::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        showNormal();
        activateWindow();
    }
}
