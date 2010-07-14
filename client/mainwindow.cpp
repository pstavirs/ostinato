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

#include "mainwindow.h"

#if 0
#include "dbgthread.h"
#endif

#include "portgrouplist.h"
#include "portstatswindow.h"
#include "portswindow.h"
#include "preferences.h"
#include "ui_about.h"

#include <QDockWidget>
#include <QProcess>

extern const char* version;
extern const char* revision;

PortGroupList    *pgl;

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow (parent)
{
    QString serverApp = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN32
    serverApp.append("/drone.exe");
#else
    serverApp.append("/drone");
#endif

    localServer_ = new QProcess(this);
    localServer_->setProcessChannelMode(QProcess::ForwardedChannels);
    localServer_->start(serverApp);
    // TODO: waitForReadyRead() is a kludge till we implement auto-retry!
    localServer_->waitForReadyRead(1000);

    pgl = new PortGroupList;

    portsWindow = new PortsWindow(pgl, this);
    statsWindow = new PortStatsWindow(pgl, this);
    portsDock = new QDockWidget(tr("Ports and Streams"), this);
    statsDock = new QDockWidget(tr("Statistics"), this);

    setupUi(this);

    menuFile->insertActions(menuFile->actions().at(0), portsWindow->actions());

    statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
    portsDock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, portsDock);

    connect(actionFileExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
#if 0
    {
        DbgThread *dbg = new DbgThread(pgl);
        dbg->start();
    }
#endif
}

MainWindow::~MainWindow()
{
    delete pgl;
    localServer_->terminate();
    localServer_->waitForFinished();
    delete localServer_;
}

void MainWindow::on_actionPreferences_triggered()
{
    Preferences *preferences = new Preferences();

    preferences->exec();

    delete preferences;
}

void MainWindow::on_actionHelpAbout_triggered()
{
    QDialog *aboutDialog = new QDialog;

    Ui::About about;
    about.setupUi(aboutDialog);
    about.versionLabel->setText(
            QString("Version: %1 Revision: %2").arg(version).arg(revision));

    aboutDialog->exec();

    delete aboutDialog;
}

