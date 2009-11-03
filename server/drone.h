#ifndef _DRONE_H
#define _DRONE_H

#include <QTcpServer>
#include <QTcpSocket>

#include "ui_drone.h"
#include "abstracthost.h"
#include "rpcserver.h"
#include "myservice.h"

class Drone : public QDialog, AbstractHost
{
     Q_OBJECT

 public:
    Ui::Drone ui;
    Drone(QDialog *parent = 0);
	void Log(const char *msg);

 private:
	RpcServer				*rpcServer;
	OstProto::OstService	*service;
 	void LogInt(const QString &msg);
}; 
#endif
