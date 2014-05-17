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
#include "settings.h"
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

#ifdef Q_OS_MAC
    // applicationDirPath() does not return bundle, but executable inside bundle
    serverApp.replace("Ostinato.app", "drone.app");
#endif

#ifdef Q_OS_WIN32
    serverApp.append("/drone.exe");
#else
    serverApp.append("/drone");
#endif

    localServer_ = new QProcess(this);
    localServer_->setProcessChannelMode(QProcess::ForwardedChannels);
    localServer_->start(serverApp, QStringList());

    pgl = new PortGroupList;

    portsWindow = new PortsWindow(pgl, this);
    statsWindow = new PortStatsWindow(pgl, this);
    portsDock = new QDockWidget(tr("Ports and Streams"), this);
    portsDock->setObjectName("portsDock");
    portsDock->setFeatures(
                portsDock->features() & ~QDockWidget::DockWidgetClosable);
    statsDock = new QDockWidget(tr("Statistics"), this);
    statsDock->setObjectName("statsDock");
    statsDock->setFeatures(
                statsDock->features() & ~QDockWidget::DockWidgetClosable);

    setupUi(this);

    menuFile->insertActions(menuFile->actions().at(0), portsWindow->actions());

    statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
    portsDock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, portsDock);

    QRect geom = appSettings->value(kApplicationWindowGeometryKey).toRect();
    if (!geom.isNull())
        setGeometry(geom);
    QByteArray layout = appSettings->value(kApplicationWindowLayout)
                            .toByteArray();
    if (layout.size())
        restoreState(layout, 0);

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
#ifdef Q_OS_WIN32
    //! \todo - find a way to terminate cleanly
    localServer_->kill();
#else    
    localServer_->terminate();
#endif

    delete pgl;

    QByteArray layout = saveState(0);
    appSettings->setValue(kApplicationWindowLayout, layout);
    appSettings->setValue(kApplicationWindowGeometryKey, geometry());

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

