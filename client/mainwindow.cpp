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

#include "jumpurl.h"
#include "logsmodel.h"
#include "logswindow.h"
#include "params.h"
#include "portgrouplist.h"
#include "portstatswindow.h"
#include "portswindow.h"
#include "preferences.h"
#include "sessionfileformat.h"
#include "settings.h"
#include "ui_about.h"
#include "updater.h"

#include "fileformat.pb.h"

#include <QDate>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN32
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif

extern const char* version;
extern const char* revision;

PortGroupList    *pgl;
LogsModel        *appLogs;

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow (parent)
{
    Updater *updater = new Updater();

    if (appParams.optLocalDrone()) {
        QString serverApp = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        // applicationDirPath() does not return bundle,
        // but executable inside bundle
        serverApp.replace("Ostinato.app", "drone.app");
#endif
#ifdef Q_OS_WIN32
        serverApp.append("/drone.exe");
#else
        serverApp.append("/drone");
#endif

        qDebug("staring local server - %s", qPrintable(serverApp));
        localServer_ = new QProcess(this);
        connect(localServer_, SIGNAL(finished(int, QProcess::ExitStatus)),
                SLOT(onLocalServerFinished(int, QProcess::ExitStatus)));
        connect(localServer_, SIGNAL(error(QProcess::ProcessError)),
                SLOT(onLocalServerError(QProcess::ProcessError)));
        localServer_->setProcessChannelMode(QProcess::ForwardedChannels);
        localServer_->start(serverApp, QStringList());
        QTimer::singleShot(5000, this, SLOT(stopLocalServerMonitor()));
    }
    else
        localServer_ = NULL;

    pgl = new PortGroupList;
    appLogs = new LogsModel(this);

    portsWindow = new PortsWindow(pgl, this);
    statsWindow = new PortStatsWindow(pgl, this);

    portsDock = new QDockWidget(tr("Ports and Streams"), this);
    portsDock->setObjectName("portsDock");
    portsDock->setFeatures(
                portsDock->features() & ~QDockWidget::DockWidgetClosable);

    statsDock = new QDockWidget(tr("Port Statistics"), this);
    statsDock->setObjectName("statsDock");
    statsDock->setFeatures(
                statsDock->features() & ~QDockWidget::DockWidgetClosable);

    logsDock_ = new QDockWidget(tr("Logs"), this);
    logsDock_->setObjectName("logsDock");
    logsDock_->setFeatures(
                logsDock_->features() & ~QDockWidget::DockWidgetClosable);
    logsWindow_ = new LogsWindow(appLogs, logsDock_);

    setupUi(this);

    menuFile->insertActions(menuFile->actions().at(3), portsWindow->actions());

    statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
    logsDock_->setWidget(logsWindow_);
    addDockWidget(Qt::BottomDockWidgetArea, logsDock_);
    tabifyDockWidget(statsDock, logsDock_);
    statsDock->show();
    statsDock->raise();

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

    // Add the "Local" Port Group
    if (appParams.optLocalDrone()) {
        PortGroup *pg = new PortGroup;
        pgl->addPortGroup(*pg);
    }

    if (appParams.argumentCount()) {
        QString fileName = appParams.argument(0);
        if (QFile::exists(fileName))
            openSession(fileName);
        else
            QMessageBox::information(NULL, qApp->applicationName(),
                    QString("File not found: " + fileName));
    }

#if 0
    {
        DbgThread *dbg = new DbgThread(pgl);
        dbg->start();
    }
#endif
}

MainWindow::~MainWindow()
{
    stopLocalServerMonitor();
    if (localServer_) {
#ifdef Q_OS_WIN32
        //! \todo - find a way to terminate cleanly
        localServer_->kill();
#else    
        localServer_->terminate();
#endif
    }

    delete pgl;

    // We don't want to save state for Stream Stats Docks - so delete them
    QList<QDockWidget*> streamStatsDocks
            = findChildren<QDockWidget*>("streamStatsDock");
    foreach(QDockWidget *dock, streamStatsDocks)
        delete dock;
    Q_ASSERT(findChildren<QDockWidget*>("streamStatsDock").size() == 0);

    QByteArray layout = saveState(0);
    appSettings->setValue(kApplicationWindowLayout, layout);
    appSettings->setValue(kApplicationWindowGeometryKey, geometry());

    if (localServer_) {
        localServer_->waitForFinished();
        delete localServer_;
    }
}

void MainWindow::openSession(QString fileName)
{
    qDebug("Open Session Action (%s)", qPrintable(fileName));

    static QString dirName;
    QStringList fileTypes = SessionFileFormat::supportedFileTypes(
                                                SessionFileFormat::kOpenFile);
    QString fileType;
    QString errorStr;
    bool ret;

    if (!fileName.isEmpty())
        goto _skip_prompt;

    if (portsWindow->portGroupCount()) {
        if (QMessageBox::question(this,
                tr("Open Session"),
                tr("Existing session will be lost. Proceed?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::No)
            goto _exit;
    }

    if (fileTypes.size())
        fileType = fileTypes.at(0);

    fileName = QFileDialog::getOpenFileName(this, tr("Open Session"),
            dirName, fileTypes.join(";;"), &fileType);
    if (fileName.isEmpty())
        goto _exit;

_skip_prompt:
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

void MainWindow::on_actionOpenSession_triggered()
{
    openSession();
}

void MainWindow::on_actionSaveSession_triggered()
{
    qDebug("Save Session Action");

    static QString fileName;
    QStringList fileTypes = SessionFileFormat::supportedFileTypes(
                                                SessionFileFormat::kSaveFile);
    QString fileType;
    QString errorStr;
    QFileDialog::Options options;

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

    // On Mac OS with Native Dialog, getSaveFileName() ignores fileType.
    // Although currently there's only one supported file type, we may
    // have more in the future
#if defined(Q_OS_MAC)
    options |= QFileDialog::DontUseNativeDialog;
#endif

    if (fileTypes.size())
        fileType = fileTypes.at(0);

_retry:
    fileName = QFileDialog::getSaveFileName(this, tr("Save Session"),
            fileName, fileTypes.join(";;"), &fileType, options);
    if (fileName.isEmpty())
        goto _exit;

    if (QFileInfo(fileName).suffix().isEmpty()) {
        QString fileExt = fileType.section(QRegExp("[\\*\\)]"), 1, 1);
        qDebug("Adding extension '%s' to '%s'",
                qPrintable(fileExt), qPrintable(fileName));
        fileName.append(fileExt);
        if (QFileInfo(fileName).exists()) {
            if (QMessageBox::warning(this, tr("Overwrite File?"), 
                QString("The file \"%1\" already exists.\n\n"
                    "Do you wish to overwrite it?")
                    .arg(QFileInfo(fileName).fileName()),
                QMessageBox::Yes|QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes)
                goto _retry;
        }
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

    // Add streamStats as tabs
    QList<QDockWidget*> streamStatsDocks
            = findChildren<QDockWidget*>("streamStatsDock");
    foreach(QDockWidget *dock, streamStatsDocks) {
        dock->setFloating(false);
        tabifyDockWidget(statsDock, dock);
    }
    statsDock->show();
    statsDock->raise();

    actionViewShowMyReservedPortsOnly->setChecked(false);
}

void MainWindow::on_actionHelpOnline_triggered()
{
    QDesktopServices::openUrl(QUrl(jumpUrl("help", "app", "menu")));
}

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl(jumpUrl("donate", "app", "menu")));
}

void MainWindow::on_actionCheckForUpdates_triggered()
{
    Updater *updater = new Updater();
    connect(updater, SIGNAL(latestVersion(QString)),
            this, SLOT(onLatestVersion(QString)));
    updater->checkForNewVersion();
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

void MainWindow::stopLocalServerMonitor()
{
    // We are only interested in startup errors
    disconnect(localServer_, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onLocalServerError(QProcess::ProcessError)));
    disconnect(localServer_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onLocalServerFinished(int, QProcess::ExitStatus)));
}

void MainWindow::onLocalServerFinished(int exitCode,
                                       QProcess::ExitStatus /*exitStatus*/)
{
    if (exitCode)
        reportLocalServerError();
}

void MainWindow::onLocalServerError(QProcess::ProcessError /*error*/)
{
    reportLocalServerError();
}

void MainWindow::reportLocalServerError()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setStyleSheet("messagebox-text-interaction-flags: 5"); // mouse copy
    QString errorStr = tr("<p>Failed to start the local drone agent - "
                          "error 0x%1, exit status 0x%2 exit code 0x%3.</p>")
            .arg(localServer_->error(), 0, 16)
            .arg(localServer_->exitStatus(), 0, 16)
            .arg(localServer_->exitCode(), 0, 16);
    if (localServer_->error() == QProcess::FailedToStart)
        errorStr.append(tr("<p>The drone program does not exist at %1 or you "
                           "don't have sufficient permissions to execute it."
                           "</p>")
                        .arg(QCoreApplication::applicationDirPath()));
    if (localServer_->exitCode() == 1)
        errorStr.append(tr("<p>The drone program was not able to bind to "
                           "TCP port 7878 - maybe a drone process is already "
                           "running?</p>"));
#ifdef Q_OS_WIN32
    if (localServer_->exitCode() == STATUS_DLL_NOT_FOUND)
        errorStr.append(tr("<p>This is most likely because Packet.dll "
                           "was not found - make sure you have "
                           "<a href='%1'>WinPcap"
                           "</a> installed.</p>")
                                .arg(jumpUrl("winpcap")));
#endif
    msgBox.setText(errorStr);
    msgBox.setInformativeText(tr("Try running drone directly."));
    msgBox.exec();

    QMessageBox::information(this, QString(),
        tr("<p>If you have remote drone agents running, you can still add "
           "and connect to them.</p>"
           "<p>If you don't want to start the local drone agent at startup, "
           "provide the <b>-c</b> option to Ostinato on the command line.</p>"
           "<p>Learn about Ostinato's <a href='%1'>Controller-Agent "
           "architecture</a></p>").arg(jumpUrl("arch")));
}

void MainWindow::onNewVersion(QString newVersion)
{
    QDate today = QDate::currentDate();
    QDate lastChecked = QDate::fromString(
                            appSettings->value(kLastUpdateCheck).toString(),
                            Qt::ISODate);
    if (lastChecked.daysTo(today) >= 5) {
        QMessageBox::information(this, tr("Update check"),
            tr("<p><b>Ostinato version %1 is now available</b> (you have %2). "
                "See <a href='%3'>change log</a>.</p>"
                "<p>Visit <a href='%4'>ostinato.org</a> to download.</p>")
                .arg(newVersion)
                .arg(version)
                .arg(jumpUrl("changelog", "app", "status", "update"))
                .arg(jumpUrl("download", "app", "status", "update")));
    }
    else {
        QLabel *msg = new QLabel(tr("New Ostinato version %1 available. Visit "
                    "<a href='%2'>ostinato.org</a> to download")
                .arg(newVersion)
                .arg(jumpUrl("download", "app", "status", "update")));
        msg->setOpenExternalLinks(true);
        statusBar()->addPermanentWidget(msg);
    }

    appSettings->setValue(kLastUpdateCheck, today.toString(Qt::ISODate));
    sender()->deleteLater();
}

void MainWindow::onLatestVersion(QString latestVersion)
{
    if (version != latestVersion) {
        QMessageBox::information(this, tr("Update check"),
            tr("<p><b>Ostinato version %1 is now available</b> (you have %2). "
                "See <a href='%3'>change log</a>.</p>"
                "<p>Visit <a href='%4'>ostinato.org</a> to download.</p>")
                .arg(latestVersion)
                .arg(version)
                .arg(jumpUrl("changelog", "app", "status", "update"))
                .arg(jumpUrl("download", "app", "status", "update")));
    }
    else {
        QMessageBox::information(this, tr("Update check"),
            tr("You are already running the latest Ostinato version - %1")
                .arg(version));
    }

    sender()->deleteLater();
}

//! Returns true on success (or user cancel) and false on failure
bool MainWindow::openSession(QString fileName, QString &error)
{
    bool ret = false;
    QDialog *optDialog;
    QProgressDialog progress("Opening Session", "Cancel", 0, 0, this);
    OstProto::SessionContent session;
    SessionFileFormat *fmt = SessionFileFormat::fileFormatFromFile(fileName);

    if (fmt == NULL) {
        error = tr("Unknown session file format");
        goto _fail;
    }

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

