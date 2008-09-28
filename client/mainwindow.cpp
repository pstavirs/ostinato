#include <QtGui>
#include "mainwindow.h"
#include "portswindow.h"
#include "portstatswindow.h"
#include "portgrouplist.h"

PortGroupList	*pgl;

MainWindow::MainWindow(QWidget *parent) 
	: QMainWindow (parent)
{
	pgl = new PortGroupList;

	PortsWindow *portsWindow = new PortsWindow(pgl, this);
	PortStatsWindow *statsWindow = new PortStatsWindow(pgl, this);
	QDockWidget *dock = new QDockWidget(tr("Ports"), this);
	QDockWidget *dock2 = new QDockWidget(tr("Stats"), this);

	setupUi(this);

	dock2->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, dock2);
	dock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, dock);
}

void MainWindow::on_actionPreferences_triggered()
{
}

