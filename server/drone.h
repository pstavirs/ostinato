#ifndef _DRONE_H
#define _DRONE_H

#include <QTcpServer>
#include <QTcpSocket>
#include "ui_drone.h"
#include "abstracthost.h"
#if 0 // PB
#include "rxtx.h"
#endif
#include "rpcserver.h"
#include "myservice.h"


class Drone : public QDialog, AbstractHost
{
     Q_OBJECT

 public:
    Ui::Drone ui;
    Drone(QDialog *parent = 0);
	void Log(const char *msg);
#if 0 // PB
	int SendMsg(const void* msg, int msgLen);
#endif

 private:
#if 0 // PB
 	RxTx		*rxtx;
#endif
	RpcServer				*rpcServer;
	OstProto::OstService	*service;
 	void LogInt(const QString &msg);
#if 0 // PB
 	QTcpServer	*server;
 	QTcpSocket	*clientSock;
#define DRONE_PORT		7878
	quint16		serverPortNum;

 private slots:
 	void when_newConnection();
 	void when_disconnected();
 	void when_dataAvail();
	void when_error(QAbstractSocket::SocketError socketError);
#endif
}; 
#endif
