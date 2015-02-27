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

#include "portgroup.h"

#include "settings.h"

#include <QApplication>
#include <QCursor>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>
#include <QtGlobal>

using ::google::protobuf::NewCallback;

extern QMainWindow *mainWindow;
extern char *version;

quint32 PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QHostAddress ip, quint16 port)
{
    // Allocate an id for self
    mPortGroupId = PortGroup::mPortGroupAllocId++;

    portIdList_ = new OstProto::PortIdList;
    portStatsList_ = new OstProto::PortStatsList;

    statsController = new PbRpcController(portIdList_, portStatsList_);
    isGetStatsPending_ = false;

    compat = kUnknown;

    reconnect = false;
    reconnectAfter = kMinReconnectWaitTime;
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, SIGNAL(timeout()), 
        this, SLOT(on_reconnectTimer_timeout()));

    rpcChannel = new PbRpcChannel(ip, port);
    serviceStub = new OstProto::OstService::Stub(rpcChannel);

    // FIXME(LOW):Can't for my life figure out why this ain't working!
    //QMetaObject::connectSlotsByName(this);
    connect(rpcChannel, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
        this, SLOT(on_rpcChannel_stateChanged(QAbstractSocket::SocketState)));
    connect(rpcChannel, SIGNAL(connected()), 
        this, SLOT(on_rpcChannel_connected()));
    connect(rpcChannel, SIGNAL(disconnected()),
        this, SLOT(on_rpcChannel_disconnected()));
    connect(rpcChannel, SIGNAL(error(QAbstractSocket::SocketError)), 
        this, SLOT(on_rpcChannel_error(QAbstractSocket::SocketError)));

    connect(this, SIGNAL(portListChanged(quint32)),
        this, SLOT(when_portListChanged(quint32)), Qt::QueuedConnection);
}

PortGroup::~PortGroup()
{
    qDebug("PortGroup Destructor");
    // Disconnect and free rpc channel etc.
    PortGroup::disconnectFromHost();
    delete serviceStub;
    delete rpcChannel;
    delete statsController;
}


// ------------------------------------------------
//                      Slots
// ------------------------------------------------
void PortGroup::on_reconnectTimer_timeout()
{
    reconnectAfter *= 2;
    if (reconnectAfter > kMaxReconnectWaitTime)
        reconnectAfter = kMaxReconnectWaitTime;

    connectToHost();
}

void PortGroup::on_rpcChannel_stateChanged(QAbstractSocket::SocketState state)
{
    qDebug("state changed %d", state);

    switch (state)
    {
        case QAbstractSocket::UnconnectedState:
        case QAbstractSocket::ClosingState:
            break;

        default:
            emit portGroupDataChanged(mPortGroupId);
    }
}

void PortGroup::on_rpcChannel_connected()
{
    OstProto::VersionInfo *verInfo = new OstProto::VersionInfo;
    OstProto::VersionCompatibility *verCompat = 
            new OstProto::VersionCompatibility;
    
    qDebug("connected\n");
    emit portGroupDataChanged(mPortGroupId);

    reconnectAfter = kMinReconnectWaitTime;

    qDebug("requesting version check ...");
    verInfo->set_version(version);
    
    PbRpcController *controller = new PbRpcController(verInfo, verCompat);
    serviceStub->checkVersion(controller, verInfo, verCompat, 
            NewCallback(this, &PortGroup::processVersionCompatibility, 
                        controller));
}

void PortGroup::processVersionCompatibility(PbRpcController *controller)
{
    OstProto::VersionCompatibility *verCompat 
        = static_cast<OstProto::VersionCompatibility*>(controller->response());

    Q_ASSERT(verCompat != NULL);

    qDebug("got version result ...");

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _error_exit;
    }

    if (verCompat->result() == OstProto::VersionCompatibility::kIncompatible) {
        qWarning("incompatible version %s (%s)", version, 
                qPrintable(QString::fromStdString(verCompat->notes())));
        compat = kIncompatible;
        emit portGroupDataChanged(mPortGroupId);
        goto _error_exit;
    }

    compat = kCompatible;

    {
        OstProto::Void *void_ = new OstProto::Void;
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;

        qDebug("requesting portlist ...");
        PbRpcController *controller = new PbRpcController(void_, portIdList);
        serviceStub->getPortIdList(controller, void_, portIdList, 
                NewCallback(this, &PortGroup::processPortIdList, controller));
    }

_error_exit:
    delete controller;
}

void PortGroup::on_rpcChannel_disconnected()
{
    qDebug("disconnected\n");
    emit portListAboutToBeChanged(mPortGroupId);

    while (!mPorts.isEmpty())
        delete mPorts.takeFirst(); 

    emit portListChanged(mPortGroupId);
    emit portGroupDataChanged(mPortGroupId);

    isGetStatsPending_ = false;

    if (reconnect)
    {
        qDebug("starting reconnect timer for %d ms ...", reconnectAfter);
        reconnectTimer->start(reconnectAfter);
    }
}

void PortGroup::on_rpcChannel_error(QAbstractSocket::SocketError socketError)
{
    qDebug("%s: error %d", __FUNCTION__, socketError);
    emit portGroupDataChanged(mPortGroupId);

    if (socketError == QAbstractSocket::RemoteHostClosedError)
        reconnect = false;

    qDebug("%s: state %d", __FUNCTION__, rpcChannel->state());
    if ((rpcChannel->state() == QAbstractSocket::UnconnectedState) && reconnect)
    {
        qDebug("starting reconnect timer for %d ms...", reconnectAfter);
        reconnectTimer->start(reconnectAfter);
    }
}

void PortGroup::when_portListChanged(quint32 /*portGroupId*/)
{
    if (state() == QAbstractSocket::ConnectedState && numPorts() <= 0)
    {
        QMessageBox::warning(NULL, tr("No ports in portgroup"),
            QString("The portgroup %1:%2 does not contain any ports!\n\n"
               "Packet Transmit/Capture requires elevated privileges. "
               "Please ensure that you are running 'drone' - the server "
               "component of Ostinato with admin/root OR setuid privilege.\n\n"
               "For more information see "
               "http://code.google.com/p/ostinato/wiki/FAQ#"
                   "Q._Port_group_has_no_interfaces")
                .arg(serverAddress().toString())
                .arg(int(serverPort())));
    }
}

void PortGroup::processPortIdList(PbRpcController *controller)
{
    OstProto::PortIdList *portIdList 
        = static_cast<OstProto::PortIdList*>(controller->response());

    Q_ASSERT(portIdList != NULL);

    qDebug("got a portlist ...");

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _error_exit;
    }

    emit portListAboutToBeChanged(mPortGroupId);

    for(int i = 0; i <  portIdList->port_id_size(); i++)
    {
        Port *p;
        
        p = new Port(portIdList->port_id(i).id(), mPortGroupId);
        connect(p, SIGNAL(portDataChanged(int, int)), 
                this, SIGNAL(portGroupDataChanged(int, int)));
        qDebug("before port append\n");
        mPorts.append(p);
    }

    emit portListChanged(mPortGroupId);

    portIdList_->CopyFrom(*portIdList);

    // Request PortConfigList
    {
        qDebug("requesting port config list ...");
        OstProto::PortIdList *portIdList2 = new OstProto::PortIdList();
        OstProto::PortConfigList *portConfigList = new OstProto::PortConfigList();
        PbRpcController *controller2 = new PbRpcController(portIdList2, 
                portConfigList);

        portIdList2->CopyFrom(*portIdList);

        serviceStub->getPortConfig(controller, portIdList2, portConfigList, 
                NewCallback(this, &PortGroup::processPortConfigList, controller2));

        goto _exit;
    }

_error_exit:
_exit:
    delete controller;
}

void PortGroup::processPortConfigList(PbRpcController *controller)
{
    OstProto::PortConfigList *portConfigList 
        = static_cast<OstProto::PortConfigList*>(controller->response());

    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _error_exit;
    }

    //emit portListAboutToBeChanged(mPortGroupId);

    for(int i = 0; i < portConfigList->port_size(); i++)
    {
        uint    id;

        id = portConfigList->port(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updatePortConfig(portConfigList->mutable_port(i));
    }

    //emit portListChanged(mPortGroupId);

    // FIXME: check if we need new signals since we are not changing the
    // number of ports, just the port data

    if (numPorts() > 0)
        getStreamIdList();

_error_exit:
    delete controller;
}

void PortGroup::when_configApply(int portIndex)
{
    OstProto::StreamIdList *streamIdList;
    OstProto::StreamConfigList *streamConfigList;
    OstProto::Ack *ack;
    PbRpcController *controller;

    Q_ASSERT(portIndex < mPorts.size());

    if (state() != QAbstractSocket::ConnectedState)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    mainWindow->setDisabled(true);

    qDebug("applying 'deleted streams' ...");
    streamIdList = new OstProto::StreamIdList;
    ack = new OstProto::Ack;
    controller = new PbRpcController(streamIdList, ack);

    streamIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getDeletedStreamsSinceLastSync(*streamIdList);

    serviceStub->deleteStream(controller, streamIdList, ack, 
        NewCallback(this, &PortGroup::processDeleteStreamAck, controller));

    qDebug("applying 'new streams' ...");
    streamIdList = new OstProto::StreamIdList;
    ack = new OstProto::Ack;
    controller = new PbRpcController(streamIdList, ack);

    streamIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getNewStreamsSinceLastSync(*streamIdList);

    serviceStub->addStream(controller, streamIdList, ack, 
        NewCallback(this, &PortGroup::processAddStreamAck, controller));

    qDebug("applying 'modified streams' ...");
    streamConfigList = new OstProto::StreamConfigList;
    ack = new OstProto::Ack;
    controller = new PbRpcController(streamConfigList, ack);

    streamConfigList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getModifiedStreamsSinceLastSync(*streamConfigList);

    serviceStub->modifyStream(controller, streamConfigList, ack, 
            NewCallback(this, &PortGroup::processModifyStreamAck, 
                portIndex, controller));
}

void PortGroup::processAddStreamAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    delete controller;
}

void PortGroup::processDeleteStreamAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    delete controller;
}

void PortGroup::processModifyStreamAck(int portIndex, 
        PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    qDebug("apply completed");
    mPorts[portIndex]->when_syncComplete();

    mainWindow->setEnabled(true);
    QApplication::restoreOverrideCursor();

    delete controller;
}

void PortGroup::modifyPort(int portIndex, OstProto::Port portConfig)
{
    OstProto::PortConfigList *portConfigList = new OstProto::PortConfigList;
    OstProto::Ack *ack = new OstProto::Ack;

    qDebug("%s: portIndex = %d", __FUNCTION__, portIndex);

    Q_ASSERT(portIndex < mPorts.size());

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    mainWindow->setDisabled(true);

    OstProto::Port *port = portConfigList->add_port();
    port->CopyFrom(portConfig);
    port->mutable_port_id()->set_id(mPorts[portIndex]->id());

    PbRpcController *controller = new PbRpcController(portConfigList, ack);
    serviceStub->modifyPort(controller, portConfigList, ack, 
        NewCallback(this, &PortGroup::processModifyPortAck, controller));
}

void PortGroup::processModifyPortAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _exit;
    }

    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::PortConfigList *portConfigList = new OstProto::PortConfigList;
        PbRpcController *controller2 = new PbRpcController(portIdList, 
                portConfigList);

        OstProto::PortId *portId = portIdList->add_port_id();
        portId->CopyFrom(static_cast<OstProto::PortConfigList*>
                (controller->request())->mutable_port(0)->port_id());

        serviceStub->getPortConfig(controller, portIdList, portConfigList, 
            NewCallback(this, &PortGroup::processUpdatedPortConfig, 
                controller2));
    }
_exit:
    delete controller;
}

void PortGroup::processUpdatedPortConfig(PbRpcController *controller)
{
    OstProto::PortConfigList *portConfigList
        = static_cast<OstProto::PortConfigList*>(controller->response());

    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _exit;
    }

    if (portConfigList->port_size() != 1)
        qDebug("port size = %d (expected = 1)", portConfigList->port_size());

    for(int i = 0; i < portConfigList->port_size(); i++)
    {
        uint    id;

        id = portConfigList->port(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updatePortConfig(portConfigList->mutable_port(i));

        emit portGroupDataChanged(mPortGroupId, id);
    }


_exit:
    mainWindow->setEnabled(true);
    QApplication::restoreOverrideCursor();
    delete controller;
}

void PortGroup::getStreamIdList()
{
    for (int portIndex = 0; portIndex < numPorts(); portIndex++)
    {
        OstProto::PortId *portId = new OstProto::PortId;
        OstProto::StreamIdList *streamIdList = new OstProto::StreamIdList;
        PbRpcController *controller = new PbRpcController(portId, streamIdList);

        portId->set_id(mPorts[portIndex]->id());

        serviceStub->getStreamIdList(controller, portId, streamIdList,
                NewCallback(this, &PortGroup::processStreamIdList, 
                    portIndex, controller));
    }
}

void PortGroup::processStreamIdList(int portIndex, PbRpcController *controller)
{
    OstProto::StreamIdList *streamIdList
        = static_cast<OstProto::StreamIdList*>(controller->response());

    qDebug("In %s (portIndex = %d)", __FUNCTION__, portIndex);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (streamIdList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            streamIdList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    for(int i = 0; i < streamIdList->stream_id_size(); i++)
    {
        uint streamId;

        streamId = streamIdList->stream_id(i).id();
        mPorts[portIndex]->insertStream(streamId);
    }

    mPorts[portIndex]->when_syncComplete();

    // Are we done for all ports?
    if (numPorts() && portIndex >= (numPorts()-1))
    {
        // FIXME(HI): some way to reset streammodel 
        getStreamConfigList();
    }

_exit:
    delete controller;
}

void PortGroup::getStreamConfigList()
{
    qDebug("requesting stream config list ...");

    for (int portIndex = 0; portIndex < numPorts(); portIndex++)
    {
        OstProto::StreamIdList *streamIdList = new OstProto::StreamIdList;
        OstProto::StreamConfigList *streamConfigList 
                = new OstProto::StreamConfigList;
        PbRpcController *controller = new PbRpcController(
                streamIdList, streamConfigList);

        streamIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
        for (int j = 0; j < mPorts[portIndex]->numStreams(); j++)
        {
            OstProto::StreamId *s = streamIdList->add_stream_id();
            s->set_id(mPorts[portIndex]->streamByIndex(j)->id());
        }

        serviceStub->getStreamConfig(controller, streamIdList, streamConfigList,
                NewCallback(this, &PortGroup::processStreamConfigList, 
                    portIndex, controller));
    }
}

void PortGroup::processStreamConfigList(int portIndex, 
        PbRpcController *controller)
{
    OstProto::StreamConfigList *streamConfigList
        = static_cast<OstProto::StreamConfigList*>(controller->response());

    qDebug("In %s", __PRETTY_FUNCTION__);

    Q_ASSERT(portIndex < numPorts());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (streamConfigList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            streamConfigList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    for(int i = 0; i <  streamConfigList->stream_size(); i++)
    {
        uint streamId;

        streamId = streamConfigList->stream(i).stream_id().id();
        mPorts[portIndex]->updateStream(streamId,
            streamConfigList->mutable_stream(i));
    }

    // Are we done for all ports?
    if (portIndex >= numPorts())
    {
        // FIXME(HI): some way to reset streammodel 
    }

_exit:
    delete controller;
}

void PortGroup::startTx(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if (portList == NULL)
        goto _exit;

    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::Ack *ack = new OstProto::Ack;
        PbRpcController *controller = new PbRpcController(portIdList, ack);

        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId *portId = portIdList->add_port_id();
            portId->set_id(portList->at(i));
        }

        serviceStub->startTransmit(controller, portIdList, ack,
                NewCallback(this, &PortGroup::processStartTxAck, controller));
    }
_exit:
    return;
}

void PortGroup::processStartTxAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    delete controller;
}

void PortGroup::stopTx(QList<uint> *portList)
{

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;
    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::Ack *ack = new OstProto::Ack;
        PbRpcController *controller = new PbRpcController(portIdList, ack);

        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId *portId = portIdList->add_port_id();
            portId->set_id(portList->at(i));
        }

        serviceStub->stopTransmit(controller, portIdList, ack,
                NewCallback(this, &PortGroup::processStopTxAck, controller));
    }
_exit:
    return;
}

void PortGroup::processStopTxAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    delete controller;
}

void PortGroup::startCapture(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;

    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::Ack *ack = new OstProto::Ack;
        PbRpcController *controller = new PbRpcController(portIdList, ack);

        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId *portId = portIdList->add_port_id();
            portId->set_id(portList->at(i));
        }

        serviceStub->startCapture(controller, portIdList, ack,
            NewCallback(this, &PortGroup::processStartCaptureAck, controller));
    }
_exit:
    return;
}

void PortGroup::processStartCaptureAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    delete controller;
}

void PortGroup::stopCapture(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;

    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::Ack *ack = new OstProto::Ack;
        PbRpcController *controller = new PbRpcController(portIdList, ack);

        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId *portId = portIdList->add_port_id();
            portId->set_id(portList->at(i));
        }

        serviceStub->stopCapture(controller, portIdList, ack,
            NewCallback(this, &PortGroup::processStopCaptureAck, controller));
    }
_exit:
    return;
}

void PortGroup::processStopCaptureAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    delete controller;
}

void PortGroup::viewCapture(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if ((portList == NULL) || (portList->size() != 1))
        goto _exit;


    for (int i = 0; i < portList->size(); i++)
    {
        OstProto::PortId *portId = new OstProto::PortId;
        OstProto::CaptureBuffer *buf = new OstProto::CaptureBuffer;
        PbRpcController *controller = new PbRpcController(portId, buf);
        QFile *capFile = mPorts[portList->at(i)]->getCaptureFile();

        portId->set_id(portList->at(i));

        capFile->open(QIODevice::ReadWrite|QIODevice::Truncate);
        qDebug("Temp CapFile = %s", capFile->fileName().toAscii().constData());
        controller->setBinaryBlob(capFile);

        serviceStub->getCaptureBuffer(controller, portId, buf,
            NewCallback(this, &PortGroup::processViewCaptureAck, controller));
    }
_exit:
    return;
}

void PortGroup::processViewCaptureAck(PbRpcController *controller)
{
    QFile *capFile = static_cast<QFile*>(controller->binaryBlob());

    QString viewer = appSettings->value(kWiresharkPathKey, 
            kWiresharkPathDefaultValue).toString();

    qDebug("In %s", __FUNCTION__);

    capFile->flush();
    capFile->close();

    if (!QFile::exists(viewer))
    {
        QMessageBox::warning(NULL, "Can't find Wireshark", 
                viewer + QString(" does not exist!\n\nPlease correct the path"
                " to Wireshark in the Preferences."));
        goto _exit;
    }

    if (!QProcess::startDetached(viewer, QStringList() << capFile->fileName()))
        qDebug("Failed starting Wireshark");

_exit:
    delete controller;
}

void PortGroup::getPortStats()
{
    //qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if (numPorts() <= 0)
        goto _exit;

    if (isGetStatsPending_)
        goto _exit;

    statsController->Reset();
    isGetStatsPending_ = true;
    serviceStub->getStats(statsController, 
        static_cast<OstProto::PortIdList*>(statsController->request()), 
        static_cast<OstProto::PortStatsList*>(statsController->response()), 
        NewCallback(this, &PortGroup::processPortStatsList));

_exit:
    return;
}

void PortGroup::processPortStatsList()
{
    //qDebug("In %s", __FUNCTION__);

    if (statsController->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(statsController->ErrorString()));
        goto _error_exit;
    }

    for(int i = 0; i < portStatsList_->port_stats_size(); i++)
    {
        uint id = portStatsList_->port_stats(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updateStats(portStatsList_->mutable_port_stats(i));
    }

    emit statsChanged(mPortGroupId);

_error_exit:
    isGetStatsPending_ = false;
}

void PortGroup::clearPortStats(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    {
        OstProto::PortIdList *portIdList = new OstProto::PortIdList;
        OstProto::Ack *ack = new OstProto::Ack;
        PbRpcController *controller = new PbRpcController(portIdList, ack);

        if (portList == NULL)
            portIdList->CopyFrom(*portIdList_);
        else
        {
            for (int i = 0; i < portList->size(); i++)
            {
                OstProto::PortId *portId = portIdList->add_port_id();
                portId->set_id(portList->at(i));
            }
        }

        serviceStub->clearStats(controller, portIdList, ack,
            NewCallback(this, &PortGroup::processClearStatsAck, controller));
    }
_exit:
    return;
}

void PortGroup::processClearStatsAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    // Refresh stats immediately after a stats clear/reset
    getPortStats();

    delete controller;
}

