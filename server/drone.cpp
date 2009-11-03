#include "drone.h"

extern int myport;

Drone::Drone(QDialog *parent)
     : QDialog(parent)
{
	ui.setupUi(this);

	rpcServer = new RpcServer();
	service = new MyService(this);
	rpcServer->registerService(service, myport ? myport : 7878);
} 

void Drone::Log(const char* str)
{
	ui.teLog->append(QString(str));
}

void Drone::LogInt(const QString &str)
{
	ui.teLog->append(str);
}
