#ifndef _PB_RPC_CHANNEL_H
#define _PB_RPC_CHANNEL_H

#include <QTcpServer>
#include <QTcpSocket>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "pbrpccommon.h"
#include "pbrpccontroller.h"

class PbRpcChannel : public QObject, public ::google::protobuf::RpcChannel
{
	Q_OBJECT
	
	// If isPending is TRUE, then  controller, done, response
	// and pendingMethodId correspond to the last method called by
	// the service stub
	bool			isPending;
	int				pendingMethodId;

	// controller, done, response are set to the corresponding values
	// passed by the stub to CallMethod(). They are reset to NULL when
	// we get a response back from the server in on_mpSocket_readyRead()
	// after calling done->Run().

	/*! \todo (MED) : change controller, done and response to references
	 instead of pointers? */
	::google::protobuf::RpcController	*controller;
	::google::protobuf::Closure			*done;
	::google::protobuf::Message			*response;

	typedef struct _RpcCall {
		const ::google::protobuf::MethodDescriptor	*method;
		::google::protobuf::RpcController		*controller;
		const ::google::protobuf::Message				*request;
		::google::protobuf::Message				*response;
		::google::protobuf::Closure				*done;
	} RpcCall;
	QList<RpcCall>		pendingCallList;

	QHostAddress	mServerAddress;
	quint16			mServerPort;
	QTcpSocket		*mpSocket;

public:
	PbRpcChannel(QHostAddress ip, quint16 port);
	~PbRpcChannel();

	void establish();
	void establish(QHostAddress ip, quint16 port);
	void tearDown();

	const QHostAddress& serverAddress() const { return mServerAddress; } 
	quint16 serverPort() const { return mServerPort; } 

	QAbstractSocket::SocketState state() const
		{ return mpSocket->state(); }	

	void CallMethod(const ::google::protobuf::MethodDescriptor *method,
		::google::protobuf::RpcController *controller,
		const ::google::protobuf::Message *req,
		::google::protobuf::Message *response,
		::google::protobuf::Closure* done);

signals:
	void connected();
	void disconnected();
	void error(QAbstractSocket::SocketError socketError);
	void stateChanged(QAbstractSocket::SocketState socketState);

private slots:
	void on_mpSocket_connected();
	void on_mpSocket_disconnected();
	void on_mpSocket_stateChanged(QAbstractSocket::SocketState socketState);
	void on_mpSocket_error(QAbstractSocket::SocketError socketError);

	void on_mpSocket_readyRead();
};

#endif
