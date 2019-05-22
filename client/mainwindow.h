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

#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QProcess>

class LogsWindow;
class PortsWindow;
class PortStatsWindow;

class QDockWidget;
class QProcess;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

private:
    void openSession(QString fileName = QString());
    bool openSession(QString fileName, QString &error);
    bool saveSession(QString fileName, QString fileType, QString &error);

    QProcess        *localServer_;
    PortsWindow        *portsWindow;
    PortStatsWindow *statsWindow;
    LogsWindow         *logsWindow_;
    QDockWidget        *portsDock;
    QDockWidget        *statsDock;
    QDockWidget        *logsDock_;

    QRect defaultGeometry_;
    QByteArray defaultLayout_;

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_actionOpenSession_triggered();
    void on_actionSaveSession_triggered();
    void on_actionPreferences_triggered();
    void on_actionViewRestoreDefaults_triggered();
    void on_actionHelpOnline_triggered();
    void on_actionDonate_triggered();
    void on_actionCheckForUpdates_triggered();
    void on_actionHelpAbout_triggered();

private slots:
    void stopLocalServerMonitor();
    void onLocalServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onLocalServerError(QProcess::ProcessError error);
    void reportLocalServerError();
    void onNewVersion(QString version);
    void onLatestVersion(QString version);
};

#endif

