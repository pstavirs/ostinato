#ifndef _DRONE_H
#define _DRONE_H

#include <QTcpServer>
#include <QTcpSocket>
#include "ui_drone.h"
#include "abstracthost.h"
#include "rxtx.h"


class Drone : public QDialog, AbstractHost
{
     Q_OBJECT

 public:
    Drone(QDialog *parent = 0);
	void Log(const char *msg);
	int SendMsg(const void* msg, int msgLen);

     Ui::Drone ui;
 private:
 	RxTx		*rxtx;
 	QTcpServer	*server;
 	QTcpSocket	*clientSock;
#define DRONE_PORT		7878
	quint16		serverPortNum;
 //    Ui::Drone ui;
 	void LogInt(const QString &msg);
 private slots:
 	void when_newConnection();
 	void when_disconnected();
 	void when_dataAvail();
	void when_error(QAbstractSocket::SocketError socketError);
}; 
#endif

