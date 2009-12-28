#include "mainwindow.h"

#if 0
#include "dbgthread.h"
#endif

#include "portgrouplist.h"
#include "portstatswindow.h"
#include "portswindow.h"
#include "ui_about.h"

#include <QDockWidget>
#include <QProcess>

PortGroupList    *pgl;

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow (parent)
{
    localServer_ = new QProcess(this);
    localServer_->start("drone.exe");

    pgl = new PortGroupList;

    portsWindow = new PortsWindow(pgl, this);
    statsWindow = new PortStatsWindow(pgl, this);
    portsDock = new QDockWidget(tr("Ports"), this);
    statsDock = new QDockWidget(tr("Stats"), this);

    setupUi(this);

    statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
    portsDock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, portsDock);

    connect(actionFileExit, SIGNAL(triggered()), this, SLOT(close()));
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

void MainWindow::on_actionHelpAbout_triggered()
{
    QDialog *aboutDialog = new QDialog;

    Ui::About about;
    about.setupUi(aboutDialog);

    aboutDialog->exec();

    delete aboutDialog;
}

