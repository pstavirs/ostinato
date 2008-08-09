#include "drone.h"

extern int myport;	// FIXME(HIGH)

Drone::Drone(QDialog *parent)
     : QDialog(parent)
{
	ui.setupUi(this);
#if 0 // PB
	rxtx = new RxTx(this);
#endif
	rpcServer = new RpcServer();
	service = new MyService(this);
	rpcServer->registerService(service, myport?myport:7878);

#if 0 // PB
	serverPortNum = DRONE_PORT;
	clientSock = NULL;

	if (myport)
		serverPortNum = myport);

	server = new QTcpServer(this);
	connect(server, SIGNAL(newConnection()), this, SLOT(when_newConnection()));
	//if (!server->listen(QHostAddress("10.0.0.1"), serverPortNum)) 
	if (!server->listen(QHostAddress::Any, serverPortNum)) 
		LogInt(tr("Unable to start the server: %1").arg(server->errorString()));
	else
		LogInt(tr("The server is running on %1:%2").arg(server->serverAddress().toString()).arg(server->serverPort())); 
#endif
} 

void Drone::Log(const char* str)
{
	ui.teLog->append(QString(str));
}

#if 0 // PB
int Drone::SendMsg(const void* msg, int size)
{
	qDebug("Inside SendMsg\n");	
	clientSock->write((char*) msg, size);
}
#endif

void Drone::LogInt(const QString &str)
{
	ui.teLog->append(str);
}

#if 0 // PB
void Drone::when_newConnection()
{
	if (clientSock)
	{
		QTcpSocket	*sock;

		LogInt(tr("already connected, no new connections will be accepted\n"));
		sock = server->nextPendingConnection();
		// TODO: Send reason msg to client
		sock->disconnectFromHost();
		goto _exit;
	}
	clientSock = server->nextPendingConnection();
	LogInt(tr("accepting new connection from %1:%2").arg(clientSock->peerAddress().toString()).arg(clientSock->peerPort()));
	connect(clientSock, SIGNAL(readyRead()), 
		this, SLOT(when_dataAvail()));
	connect(clientSock, SIGNAL(disconnected()), 
		this, SLOT(when_disconnected()));
	connect(clientSock, SIGNAL(error(QAbstractSocket::SocketError)), 
		this, SLOT(when_error(QAbstractSocket::SocketError)));


_exit:
	return;
}

void Drone::when_disconnected()
{
	LogInt(tr("closing connection from %1:%2").arg(clientSock->peerAddress().toString()).arg(clientSock->peerPort()));
	clientSock->deleteLater();
	clientSock = NULL;
}

void Drone::when_dataAvail()
{
	QByteArray	msg = clientSock->read(1024);	// FIXME: hardcoding
	LogInt(QString(msg.toHex()));
	rxtx->ProcessMsg(msg.constData(), msg.size());
}

void Drone::when_error(QAbstractSocket::SocketError socketError)
{
	LogInt(clientSock->errorString());
}
#endif
