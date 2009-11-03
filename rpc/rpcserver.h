#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include <QTcpServer>
#include <QTcpSocket>

#include "pbrpccommon.h"
#include "pbrpccontroller.h"


class RpcServer : public QObject
{
	Q_OBJECT

	QTcpServer	*server;
	QTcpSocket	*clientSock;

	::google::protobuf::Service		*service;

	bool	isPending;
	int		pendingMethodId;

	void 	LogInt (QString log) {qDebug("%s", log.toAscii().data());}

public:
	RpcServer();	//! \todo (LOW) use 'parent' param
	virtual ~RpcServer();

	bool registerService(::google::protobuf::Service *service,
		quint16 tcpPortNum);
	void done(::google::protobuf::Message *resp, PbRpcController *controller);

private slots:
 	void when_newConnection();
 	void when_disconnected();
 	void when_dataAvail();
	void when_error(QAbstractSocket::SocketError socketError);
};

#endif
