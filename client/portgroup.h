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

#ifndef _PORT_GROUP_H
#define _PORT_GROUP_H

#include "port.h"
#include <QHostAddress>
#include <QTcpSocket>

#include "../common/protocol.pb.h"
#include "pbrpcchannel.h"

/* TODO
HIGH
MED
LOW
- Allow hostnames in addition to IP Address as "server address"
*/

#define DEFAULT_SERVER_PORT        7878

class QFile;
class QTimer;

class PortGroup : public QObject {
    Q_OBJECT

private:
    static quint32 mPortGroupAllocId;
    quint32        mPortGroupId;
    QString        mUserAlias;            // user defined

    bool            reconnect;
    int             reconnectAfter;     // time in milliseconds
    static const int kMinReconnectWaitTime = 2000; // ms
    static const int kMaxReconnectWaitTime = 60000; // ms
    QTimer          *reconnectTimer;
    PbRpcChannel    *rpcChannel;
    PbRpcController *statsController;
    bool            isGetStatsPending_;

    OstProto::OstService::Stub *serviceStub;

    OstProto::PortIdList       *portIdList_;
    OstProto::PortStatsList    *portStatsList_;

public: // FIXME(HIGH): member access
    QList<Port*>        mPorts;

public:
    PortGroup(QHostAddress ip = QHostAddress::LocalHost, 
        quint16 port = DEFAULT_SERVER_PORT); 
    ~PortGroup();

    void connectToHost() { reconnect = true; rpcChannel->establish(); }
    void connectToHost(QHostAddress ip, quint16 port) 
        { reconnect = true; rpcChannel->establish(ip, port); }
    void disconnectFromHost() { reconnect = false; rpcChannel->tearDown(); }

    int numPorts() const { return mPorts.size(); }
    quint32 id() const { return mPortGroupId; } 

    const QString& userAlias() const { return mUserAlias; } 
    void setUserAlias(QString alias) { mUserAlias = alias; };

    const QHostAddress& serverAddress() const 
        { return rpcChannel->serverAddress(); } 
    quint16 serverPort() const 
        { return rpcChannel->serverPort(); } 
    QAbstractSocket::SocketState state() const
        { return rpcChannel->state(); }    

    void processPortIdList(PbRpcController *controller);
    void processPortConfigList(PbRpcController *controller);

    void processAddStreamAck(PbRpcController *controller);
    void processDeleteStreamAck(PbRpcController *controller);
    void processModifyStreamAck(int portIndex, PbRpcController *controller);

    void modifyPort(int portId, OstProto::Port portConfig);
    void processModifyPortAck(PbRpcController *controller);
    void processUpdatedPortConfig(PbRpcController *controller);

    void getStreamIdList();
    void processStreamIdList(int portIndex, PbRpcController *controller);
    void getStreamConfigList();
    void processStreamConfigList(int portIndex, PbRpcController *controller);

    void processModifyStreamAck(OstProto::Ack *ack);

    void startTx(QList<uint> *portList = NULL);
    void processStartTxAck(PbRpcController *controller);
    void stopTx(QList<uint> *portList = NULL);
    void processStopTxAck(PbRpcController *controller);

    void startCapture(QList<uint> *portList = NULL);
    void processStartCaptureAck(PbRpcController *controller);
    void stopCapture(QList<uint> *portList = NULL);
    void processStopCaptureAck(PbRpcController *controller);
    void viewCapture(QList<uint> *portList = NULL);
    void processViewCaptureAck(PbRpcController *controller);

    void getPortStats();
    void processPortStatsList();
    void clearPortStats(QList<uint> *portList = NULL);
    void processClearStatsAck(PbRpcController *controller);

signals:
    void portGroupDataChanged(int portGroupId, int portId = 0xFFFF);
    void portListAboutToBeChanged(quint32 portGroupId);
    void portListChanged(quint32 portGroupId);
    void statsChanged(quint32 portGroupId);

private slots:
    void on_reconnectTimer_timeout();
    void on_rpcChannel_stateChanged(QAbstractSocket::SocketState state);
    void on_rpcChannel_connected();
    void on_rpcChannel_disconnected();
    void on_rpcChannel_error(QAbstractSocket::SocketError socketError);

    void when_portListChanged(quint32 portGroupId);

public slots:
    void when_configApply(int portIndex);

};

#endif
