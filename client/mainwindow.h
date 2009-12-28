#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>

class PortsWindow;
class PortStatsWindow;

class QDockWidget;
class QProcess;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

private:
    QProcess        *localServer_;
    PortsWindow        *portsWindow;
    PortStatsWindow *statsWindow;
    QDockWidget        *portsDock;
    QDockWidget        *statsDock;

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_actionHelpAbout_triggered();
};

#endif

