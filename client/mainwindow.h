#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <QMainWindow>
#include <QDockWidget>

#include "ui_mainwindow.h"

#include "portswindow.h"
#include "portstatswindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

private:
	PortsWindow		*portsWindow;
	PortStatsWindow *statsWindow;
	QDockWidget		*portsDock;
	QDockWidget		*statsDock;

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:
	void on_actionHelpAbout_triggered();
};

#endif

