#ifndef _DRONE_H
#define _DRONE_H

#include "ui_drone.h"

#include <QMenu>
#include <QSystemTrayIcon>

class RpcServer;
namespace OstProto { class OstService; }

class Drone : public QWidget, Ui::Drone 
{
     Q_OBJECT

public:
    Drone(QWidget *parent = 0);
    ~Drone();
    bool init();

signals:
    void hideMe(bool hidden);

protected:
    void changeEvent(QEvent *event);

private:
    QSystemTrayIcon            *trayIcon_;
    QMenu                    *trayIconMenu_;
    RpcServer               *rpcServer;
    OstProto::OstService    *service;

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

}; 
#endif
