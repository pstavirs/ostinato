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
#include "sessionfileformat.h"
#include "settings.h"
#include "ui_about.h"
#include "updater.h"

#include "fileformat.pb.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>

extern const char* version;
extern const char* revision;

PortGroupList    *pgl;

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow (parent)
{
    QString serverApp = QCoreApplication::applicationDirPath();
    Updater *updater = new Updater();

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

    menuFile->insertActions(menuFile->actions().at(3), portsWindow->actions());

    statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
    portsDock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, portsDock);

    // Save the default window geometry and layout ...
    defaultGeometry_ = geometry();
    defaultLayout_ = saveState(0);

    // ... before restoring the last used settings
    QRect geom = appSettings->value(kApplicationWindowGeometryKey).toRect();
    if (!geom.isNull())
        setGeometry(geom);
    QByteArray layout = appSettings->value(kApplicationWindowLayout)
                            .toByteArray();
    if (layout.size())
        restoreState(layout, 0);

    connect(actionFileExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(actionViewShowMyReservedPortsOnly, SIGNAL(toggled(bool)),
            portsWindow, SLOT(showMyReservedPortsOnly(bool)));
    connect(actionViewShowMyReservedPortsOnly, SIGNAL(toggled(bool)),
            statsWindow, SLOT(showMyReservedPortsOnly(bool)));

    connect(updater, SIGNAL(newVersionAvailable(QString)), 
            this, SLOT(onNewVersion(QString)));
    updater->checkForNewVersion();
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

void MainWindow::on_actionOpenSession_triggered()
{
    qDebug("Open Session Action");

    static QString dirName;
    QString fileName;
    QString errorStr;
    bool ret;

    fileName = QFileDialog::getOpenFileName(this, tr("Open Session"), dirName);
    if (fileName.isEmpty())
        goto _exit;

    if (portsWindow->portGroupCount()) {
        if (QMessageBox::question(this,
                tr("Open Session"),
                tr("Existing session will be lost. Proceed?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::No)
            goto _exit;
    }

    ret = openSession(fileName, errorStr);
    if (!ret || !errorStr.isEmpty()) {
        QMessageBox msgBox(this);
        QStringList str = errorStr.split("\n\n\n\n");

        msgBox.setIcon(ret ? QMessageBox::Warning : QMessageBox::Critical);
        msgBox.setWindowTitle(qApp->applicationName());
        msgBox.setText(str.at(0));
        if (str.size() > 1)
            msgBox.setDetailedText(str.at(1));
        msgBox.setStandardButtons(QMessageBox::Ok);

        msgBox.exec();
    }
    dirName = QFileInfo(fileName).absolutePath();

_exit:
    return;
}

void MainWindow::on_actionSaveSession_triggered()
{
    qDebug("Save Session Action");

    static QString fileName;
    QStringList fileTypes = SessionFileFormat::supportedFileTypes();
    QString fileType;
    QString errorStr;
    QFileDialog::Options options;

    // On Mac OS with Native Dialog, getSaveFileName() ignores fileType.
    // Although currently there's only one supported file type, we may
    // have more in the future
#if defined(Q_OS_MAC)
    options |= QFileDialog::DontUseNativeDialog;
#endif

    if (fileTypes.size())
        fileType = fileTypes.at(0);

    fileName = QFileDialog::getSaveFileName(this, tr("Save Session"),
            fileName, fileTypes.join(";;"), &fileType, options);
    if (fileName.isEmpty())
        goto _exit;

    if (portsWindow->reservedPortCount()) {
        QString myself = appSettings->value(kUserKey, kUserDefaultValue)
                            .toString();
        if (QMessageBox::question(this,
                tr("Save Session"),
                QString("Some ports are reserved!\n\nOnly ports reserved by %1 will be saved. Proceed?").arg(myself),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::No)
            goto _exit;
    }

    if (!saveSession(fileName, fileType, errorStr))
        QMessageBox::critical(this, qApp->applicationName(), errorStr);
    else if (!errorStr.isEmpty())
        QMessageBox::warning(this, qApp->applicationName(), errorStr);

    fileName = QFileInfo(fileName).absolutePath();
_exit:
    return;
}

void MainWindow::on_actionPreferences_triggered()
{
    Preferences *preferences = new Preferences();

    preferences->exec();

    delete preferences;
}

void MainWindow::on_actionViewRestoreDefaults_triggered()
{
    // Use the saved default geometry/layout, however keep the
    // window location same
    defaultGeometry_.moveTo(geometry().topLeft());
    setGeometry(defaultGeometry_);
    restoreState(defaultLayout_, 0);

    actionViewShowMyReservedPortsOnly->setChecked(false);
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

void MainWindow::onNewVersion(QString newVersion)
{
    statusBar()->showMessage(QString("New Ostinato version %1 available. "
                "Visit http://ostinato.org to download").arg(newVersion));
}

//! Returns true on success (or user cancel) and false on failure
bool MainWindow::openSession(QString fileName, QString &error)
{
    bool ret = false;
    QDialog *optDialog;
    QProgressDialog progress("Opening Session", "Cancel", 0, 0, this);
    OstProto::SessionContent session;
    SessionFileFormat *fmt = SessionFileFormat::fileFormatFromFile(fileName);

    if (fmt == NULL)
        goto _fail;

    if ((optDialog = fmt->openOptionsDialog()))
    {
        int ret;
        optDialog->setParent(this, Qt::Dialog);
        ret = optDialog->exec();
        optDialog->setParent(0, Qt::Dialog);
        if (ret == QDialog::Rejected)
            goto _user_opt_cancel;
    }

    progress.setAutoReset(false);
    progress.setAutoClose(false);
    progress.setMinimumDuration(0);
    progress.show();

    setDisabled(true);
    progress.setEnabled(true); // to override the mainWindow disable

    connect(fmt, SIGNAL(status(QString)),&progress,SLOT(setLabelText(QString)));
    connect(fmt, SIGNAL(target(int)), &progress, SLOT(setMaximum(int)));
    connect(fmt, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    connect(&progress, SIGNAL(canceled()), fmt, SLOT(cancel()));

    fmt->openAsync(fileName, session, error);
    qDebug("after open async");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for async operation");

    if (!fmt->result())
        goto _fail;

    // process any remaining events posted from the thread
    for (int i = 0; i < 10; i++)
        qApp->processEvents();

    // XXX: user can't cancel operation from here on!
    progress.close();

    portsWindow->openSession(&session, error);

_user_opt_cancel:
    ret = true;

_fail:
    progress.close();
    setEnabled(true);
    return ret;
}

bool MainWindow::saveSession(QString fileName, QString fileType, QString &error)
{
    bool ret = false;
    QProgressDialog progress("Saving Session", "Cancel", 0, 0, this);
    SessionFileFormat *fmt = SessionFileFormat::fileFormatFromType(fileType);
    OstProto::SessionContent session;

    if (fmt == NULL)
        goto _fail;

    progress.setAutoReset(false);
    progress.setAutoClose(false);
    progress.setMinimumDuration(0);
    progress.show();

    setDisabled(true);
    progress.setEnabled(true); // to override the mainWindow disable

    // Fill in session
    ret = portsWindow->saveSession(&session, error, &progress);
    if (!ret)
        goto _user_cancel;

    connect(fmt, SIGNAL(status(QString)),&progress,SLOT(setLabelText(QString)));
    connect(fmt, SIGNAL(target(int)), &progress, SLOT(setMaximum(int)));
    connect(fmt, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    connect(&progress, SIGNAL(canceled()), fmt, SLOT(cancel()));

    fmt->saveAsync(session, fileName, error);
    qDebug("after save async");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for async operation");

    ret = fmt->result();
    goto _exit;

_user_cancel:
   goto _exit;

_fail:
    error = QString("Unsupported File Type - %1").arg(fileType);
    goto _exit;

_exit:
    progress.close();
    setEnabled(true);
    return ret;
}

