/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _PB_RPC_CHANNEL_H
#define _PB_RPC_CHANNEL_H

#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

#include <google/protobuf/io/zero_copy_stream_impl.h>
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
    bool            isPending;
    int                pendingMethodId;

    // controller, done, response are set to the corresponding values
    // passed by the stub to CallMethod(). They are reset to NULL when
    // we get a response back from the server in on_mpSocket_readyRead()
    // after calling done->Run().

    /*! \todo (MED) : change controller, done and response to references
     instead of pointers? */
    const ::google::protobuf::MethodDescriptor *method;
    ::google::protobuf::RpcController    *controller;
    ::google::protobuf::Closure            *done;
    ::google::protobuf::Message            *response;

    typedef struct _RpcCall {
        const ::google::protobuf::MethodDescriptor    *method;
        ::google::protobuf::RpcController        *controller;
        const ::google::protobuf::Message                *request;
        ::google::protobuf::Message                *response;
        ::google::protobuf::Closure                *done;
    } RpcCall;
    QList<RpcCall>        pendingCallList;

    const ::google::protobuf::Message   &notifPrototype;
    ::google::protobuf::Message     *notif;

    QString            mServerHost;
    quint16            mServerPort;
    QTcpSocket        *mpSocket;

    ::google::protobuf::io::CopyingInputStreamAdaptor  *inStream;
    ::google::protobuf::io::CopyingOutputStreamAdaptor *outStream;

public:
    PbRpcChannel(QString serverName, quint16 port,
                 const ::google::protobuf::Message &notifProto);
    ~PbRpcChannel();

    void establish();
    void establish(QString serverName, quint16 port);
    void tearDown();

    const QString serverName() const
    {
        return mpSocket->peerName();
    }
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

    void notification(int notifType, ::google::protobuf::Message *notifData);

private slots:
    void on_mpSocket_connected();
    void on_mpSocket_disconnected();
    void on_mpSocket_stateChanged(QAbstractSocket::SocketState socketState);
    void on_mpSocket_error(QAbstractSocket::SocketError socketError);

    void on_mpSocket_readyRead();
};

#endif
