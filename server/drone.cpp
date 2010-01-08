#include "drone.h"

#include "rpcserver.h"
#include "myservice.h"

#include <QCloseEvent>
#include <QMessageBox>

extern int myport;

Drone::Drone(QWidget *parent)
     : QWidget(parent)
{
    setupUi(this);

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
