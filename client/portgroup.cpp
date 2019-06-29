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

#include "jumpurl.h"
#include "log.h"
#include "settings.h"

#include "emulproto.pb.h"
#include "fileformat.pb.h"

#include <QApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QRegExp>
#include <QTemporaryFile>
#include <QTimer>
#include <QtGlobal>
#include <QUrl>

using ::google::protobuf::NewCallback;

extern QMainWindow *mainWindow;
extern char *version;

quint32 PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QString serverName, quint16 port)
{
    // Allocate an id for self
    mPortGroupId = PortGroup::mPortGroupAllocId++;

    portIdList_ = new OstProto::PortIdList;
    portStatsList_ = new OstProto::PortStatsList;

    statsController = new PbRpcController(portIdList_, portStatsList_);
    isGetStatsPending_ = false;

    atConnectConfig_ = NULL;

    compat = kUnknown;

    reconnect = false;
    reconnectAfter = kMinReconnectWaitTime;
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, SIGNAL(timeout()), 
        this, SLOT(on_reconnectTimer_timeout()));

    rpcChannel = new PbRpcChannel(serverName, port,
                                  OstProto::Notification::default_instance());
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

    connect(rpcChannel, 
        SIGNAL(notification(int, ::google::protobuf::Message*)), 
        this, 
        SLOT(on_rpcChannel_notification(int, ::google::protobuf::Message*)));

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
    delete atConnectConfig_;
}

void PortGroup::setConfigAtConnect(const OstProto::PortGroupContent *config)
{
    if (!config) {
        delete atConnectConfig_;
        atConnectConfig_ = NULL;
        return;
    }

    if (!atConnectConfig_)
        atConnectConfig_ = new OstProto::PortGroupContent;
    atConnectConfig_->CopyFrom(*config);
}

int PortGroup::numReservedPorts() const
{
    int count = 0;
    for (int i = 0; i < mPorts.size(); i++)
    {
        if (!mPorts[i]->userName().isEmpty())
            count++;
    }

    return count;
}

const QString PortGroup::serverFullName() const
{
    return serverPort() == DEFAULT_SERVER_PORT ?
        serverName() : QString("%1:%2").arg(serverName()).arg(serverPort());
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
    logInfo(id(), "PortGroup connected");
    emit portGroupDataChanged(mPortGroupId);

    reconnectAfter = kMinReconnectWaitTime;

    qDebug("requesting version check ...");
    verInfo->set_client_name("ostinato");
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
        logError(id(), QString("checkVersion RPC failed: %1")
                            .arg(controller->ErrorString()));
        goto _error_exit;
    }

    if (verCompat->result() == OstProto::VersionCompatibility::kIncompatible) {
        qWarning("incompatible version %s (%s)", version, 
                qPrintable(QString::fromStdString(verCompat->notes())));
        logError(id(), QString("checkVersion failed: %1")
                            .arg(QString::fromStdString(verCompat->notes())));
        compat = kIncompatible;
        emit portGroupDataChanged(mPortGroupId);

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setStyleSheet("messagebox-text-interaction-flags: 5");
        msgBox.setText(tr("The Drone agent at %1:%2 is incompatible with this "
                          "Ostinato version - %3")
                              .arg(serverName())
                              .arg(int(serverPort()))
                              .arg(version));
        msgBox.setInformativeText(QString::fromStdString(verCompat->notes()));
        msgBox.exec();

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
    logError(id(), "PortGroup disconnected");
    emit portListAboutToBeChanged(mPortGroupId);

    while (!mPorts.isEmpty())
        delete mPorts.takeFirst(); 
    atConnectPortConfig_.clear();

    emit portListChanged(mPortGroupId);
    emit portGroupDataChanged(mPortGroupId);

    isGetStatsPending_ = false;

    if (reconnect)
    {
        qDebug("starting reconnect timer for %d ms ...", reconnectAfter);
        logInfo(id(), QString("Reconnect attempt after %1s")
                            .arg(double(reconnectAfter)/1000.0));
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
        logInfo(id(), QString("Reconnect attempt after %1s")
                            .arg(double(reconnectAfter)/1000.0));
        reconnectTimer->start(reconnectAfter);
    }
}

void PortGroup::on_rpcChannel_notification(int notifType, 
        ::google::protobuf::Message *notification)
{
    OstProto::Notification *notif = 
        dynamic_cast<OstProto::Notification*>(notification);

    if (!notif) {
        qWarning("unable to dynamic cast notif");
        return;
    }

    if (notifType != notif->notif_type()) {
        qWarning("notif type mismatch %d/%d msg = %s",
                notifType, notif->notif_type(),
                notification->DebugString().c_str());
        return;
    }

    switch (notifType) 
    {
        case OstProto::portConfigChanged: {

            if (!notif->port_id_list().port_id_size()) {
                qWarning("notif(portConfigChanged) has an empty port_id_list");
                return;
            }
            for(int i=0; i < notif->port_id_list().port_id_size(); i++) {
                logInfo(id(), notif->port_id_list().port_id(i).id(),
                        QString("Port configuration changed notification"));
            }

            OstProto::PortIdList *portIdList = new OstProto::PortIdList;
            OstProto::PortConfigList *portConfigList = 
                                            new OstProto::PortConfigList;
            PbRpcController *controller = new PbRpcController(portIdList, 
                                                           portConfigList);

            portIdList->CopyFrom(notif->port_id_list());
            serviceStub->getPortConfig(controller, portIdList, portConfigList, 
                NewCallback(this, &PortGroup::processUpdatedPortConfig, 
                    controller));
            break;
        }
        default:
            break;
    }
}

void PortGroup::when_portListChanged(quint32 /*portGroupId*/)
{
    if (state() == QAbstractSocket::ConnectedState && numPorts() <= 0)
    {
        logError(id(), QString("No ports in portlist"));
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setStyleSheet("messagebox-text-interaction-flags: 5");
        QString msg = tr("<p>The portgroup %1:%2 does not contain any ports!<p>"
               "<p>Packet Transmit/Capture requires special privileges. "
               "Please ensure that you are running 'drone' - the agent "
               "component of Ostinato with required privileges.<p>")
                .arg(serverName())
                .arg(int(serverPort()));
        msgBox.setText(msg);
        msgBox.setInformativeText(tr("See the <a href='%1'>Ostinato FAQ</a> "
                "for instructions to fix this problem").arg(jumpUrl("noports")));
        msgBox.exec();
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
        logError(id(), controller->ErrorString());
        goto _error_exit;
    }

    emit portListAboutToBeChanged(mPortGroupId);

    for(int i = 0; i <  portIdList->port_id_size(); i++)
    {
        Port *p;
        
        p = new Port(portIdList->port_id(i).id(), mPortGroupId);
        connect(p, SIGNAL(portDataChanged(int, int)), 
                this, SIGNAL(portGroupDataChanged(int, int)));
        connect(p, SIGNAL(localConfigChanged(int, int, bool)),
                this, SIGNAL(portGroupDataChanged(int, int)));
        qDebug("before port append\n");
        mPorts.append(p);
        atConnectPortConfig_.append(NULL); // will be filled later
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
        logError(id(), controller->ErrorString());
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

    // FIXME: Ideally we should emit portGroupDataChanged since only 
    // port data is changing; but writing the corresponding slot in 
    // PortStatsModel for that signal turned out to be very bug prone 
    // causing assert failures when portgroups are added/deleted or
    // connected/disconnected in different orders
    // TODO: Revisit this when we refactor the domain-objects/model/view 
    // design
    emit portListChanged(mPortGroupId);

    if (numPorts() > 0) {
        // XXX: The open session code (atConnectConfig_ related) assumes
        // the following two RPCs are invoked in the below order
        // Any change here without coressponding change in that code
        // will break stuff
        getDeviceGroupIdList();
        getStreamIdList();
    }

    // Now that we have the port details, let's identify which ports
    // need to be re-configured based on atConnectConfig_
    if (atConnectConfig_ && numPorts() > 0)
    {
        QString myself = appSettings->value(kUserKey, kUserDefaultValue)
                            .toString();
        for (int i = 0; i < atConnectConfig_->ports_size(); i++)
        {
            const OstProto::PortContent *pc = &atConnectConfig_->ports(i);
            for (int j = 0; j < mPorts.size(); j++)
            {
                Port *port = mPorts[j];

                if (port->name() == pc->port_config().name().c_str())
                {
                    if (!port->userName().isEmpty() // rsvd?
                            && port->userName() != myself) // by someone else?
                    {
                        logWarn(id(), j, QString("Port is reserved by %1. "
                                                    "Skipping reconfiguration")
                                                 .arg(port->userName()));
                        QString warning =
                                QString("%1 - %2: %3 is reserved by %4.\n\n"
                                        "Port will not be reconfigured.")
                                    .arg(serverFullName())
                                    .arg(j)
                                    .arg(port->userAlias())
                                    .arg(port->userName());
                        qWarning("%s", qPrintable(warning));
                        QMessageBox::warning(NULL, tr("Open Session"), warning);
                        continue;
                    }
                    atConnectPortConfig_[j] = pc;
                    qDebug("port %d will be reconfigured", j);
                    break;
                }

            }
        }
    }

_error_exit:
    delete controller;
}

void PortGroup::when_configApply(int portIndex)
{
    OstProto::StreamIdList *streamIdList;
    OstProto::StreamConfigList *streamConfigList;
    OstProto::BuildConfig *buildConfig;
    OstProto::Ack *ack;
    PbRpcController *controller;

    Q_ASSERT(portIndex < mPorts.size());

    if (state() != QAbstractSocket::ConnectedState)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    mainWindow->setDisabled(true);

    applyTimer_.start();

    //
    // Update/Sync DeviceGroups
    //
    OstProto::DeviceGroupIdList *deviceGroupIdList;
    OstProto::DeviceGroupConfigList *deviceGroupConfigList;
    bool refreshReqd = false;

    qDebug("applying 'deleted deviceGroups' ...");
    deviceGroupIdList = new OstProto::DeviceGroupIdList;
    deviceGroupIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getDeletedDeviceGroupsSinceLastSync(*deviceGroupIdList);
    if (deviceGroupIdList->device_group_id_size()) {
        logInfo(id(), mPorts[portIndex]->id(),
                QString("Deleting old DeviceGroups"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(deviceGroupIdList, ack);
        serviceStub->deleteDeviceGroup(controller, deviceGroupIdList, ack,
            NewCallback(this, &PortGroup::processDeleteDeviceGroupAck,
                        controller));
        refreshReqd = true;
    }
    else
        delete deviceGroupIdList;

    qDebug("applying 'new deviceGroups' ...");
    deviceGroupIdList = new OstProto::DeviceGroupIdList;
    deviceGroupIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getNewDeviceGroupsSinceLastSync(*deviceGroupIdList);
    if (deviceGroupIdList->device_group_id_size()) {
        logInfo(id(), mPorts[portIndex]->id(),
                QString("Creating new DeviceGroups"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(deviceGroupIdList, ack);
        serviceStub->addDeviceGroup(controller, deviceGroupIdList, ack,
            NewCallback(this, &PortGroup::processAddDeviceGroupAck,
                        controller));
        refreshReqd = true;
    }
    else
        delete deviceGroupIdList;

    qDebug("applying 'modified deviceGroups' ...");
    deviceGroupConfigList = new OstProto::DeviceGroupConfigList;
    deviceGroupConfigList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getModifiedDeviceGroupsSinceLastSync(
            *deviceGroupConfigList);
    if (deviceGroupConfigList->device_group_size()) {
        logInfo(id(), mPorts[portIndex]->id(),
                QString("Modifying changed DeviceGroups"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(deviceGroupConfigList, ack);
        serviceStub->modifyDeviceGroup(controller, deviceGroupConfigList, ack,
                NewCallback(this, &PortGroup::processModifyDeviceGroupAck,
                    portIndex, controller));
        refreshReqd = true;
    }
    else
        delete deviceGroupConfigList;

    if (refreshReqd)
        getDeviceInfo(portIndex);

    //
    // Update/Sync Streams
    //
    qDebug("applying 'deleted streams' ...");
    streamIdList = new OstProto::StreamIdList;
    streamIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getDeletedStreamsSinceLastSync(*streamIdList);
    if (streamIdList->stream_id_size()) {
        logInfo(id(), mPorts[portIndex]->id(), QString("Deleting old Streams"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(streamIdList, ack);
        serviceStub->deleteStream(controller, streamIdList, ack,
            NewCallback(this, &PortGroup::processDeleteStreamAck, controller));
    }
    else
        delete streamIdList;

    qDebug("applying 'new streams' ...");
    streamIdList = new OstProto::StreamIdList;
    streamIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getNewStreamsSinceLastSync(*streamIdList);
    if (streamIdList->stream_id_size()) {
        logInfo(id(), mPorts[portIndex]->id(), QString("Creating new Streams"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(streamIdList, ack);
        serviceStub->addStream(controller, streamIdList, ack,
                NewCallback(this, &PortGroup::processAddStreamAck, controller));
    }
    else
        delete streamIdList;

    qDebug("applying 'modified streams' ...");
    streamConfigList = new OstProto::StreamConfigList;
    streamConfigList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    mPorts[portIndex]->getModifiedStreamsSinceLastSync(*streamConfigList);
    if (streamConfigList->stream_size()) {
        logInfo(id(), mPorts[portIndex]->id(),
                QString("Modifying changed Streams"));
        ack = new OstProto::Ack;
        controller = new PbRpcController(streamConfigList, ack);
        serviceStub->modifyStream(controller, streamConfigList, ack,
                NewCallback(this, &PortGroup::processModifyStreamAck,
                    portIndex, controller));
    }
    else
        delete streamConfigList;

    qDebug("resolve neighbors before building ...");
    logInfo(id(), mPorts[portIndex]->id(),
            QString("Resolving device neighbors"));
    OstProto::PortIdList *portIdList = new OstProto::PortIdList;
    OstProto::PortId *portId = portIdList->add_port_id();
    portId->set_id(mPorts[portIndex]->id());
    ack = new OstProto::Ack;
    controller = new PbRpcController(portIdList, ack);
    serviceStub->resolveDeviceNeighbors(controller, portIdList, ack,
        NewCallback(this, &PortGroup::processResolveDeviceNeighborsAck,
                    controller));

    qDebug("finish apply by building ...");
    logInfo(id(), mPorts[portIndex]->id(),
            QString("Re-building packets"));
    buildConfig = new OstProto::BuildConfig;
    ack = new OstProto::Ack;
    controller = new PbRpcController(buildConfig, ack);

    buildConfig->mutable_port_id()->set_id(mPorts[portIndex]->id());
    serviceStub->build(controller, buildConfig, ack,
            NewCallback(this, &PortGroup::processApplyBuildAck,
                portIndex, controller));
}

void PortGroup::processAddDeviceGroupAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::DeviceGroupIdList *dgidList
            = static_cast<OstProto::DeviceGroupIdList*>(controller->request());
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), dgidList->port_id().id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), dgidList->port_id().id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processDeleteDeviceGroupAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::DeviceGroupIdList *dgidList
            = static_cast<OstProto::DeviceGroupIdList*>(controller->request());
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), dgidList->port_id().id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), dgidList->port_id().id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processModifyDeviceGroupAck(int /*portIndex*/,
        PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::DeviceGroupIdList *dgidList
            = static_cast<OstProto::DeviceGroupIdList*>(controller->request());
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), dgidList->port_id().id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), dgidList->port_id().id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processAddStreamAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    OstProto::StreamIdList *streamIdList = static_cast<OstProto::StreamIdList*>(
                                                controller->request());
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), streamIdList->port_id().id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), streamIdList->port_id().id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processDeleteStreamAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    OstProto::StreamIdList *streamIdList = static_cast<OstProto::StreamIdList*>(
                                                controller->request());
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), streamIdList->port_id().id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), streamIdList->port_id().id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processModifyStreamAck(int portIndex,
        PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), mPorts[portIndex]->id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    delete controller;
}

void PortGroup::processApplyBuildAck(int portIndex, PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    qDebug("apply completed");
    logInfo(id(), mPorts[portIndex]->id(),
            QString("All port changes applied - in %1s")
                .arg(applyTimer_.elapsed()/1e3));

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _error_exit;
    }

    if (ack->status())
        logError(id(), mPorts[portIndex]->id(),
                 QString::fromStdString(ack->notes()));

_error_exit:
    mPorts[portIndex]->when_syncComplete();
    emit applyFinished();

    mainWindow->setEnabled(true);
    QApplication::restoreOverrideCursor();
    delete controller;
}

void PortGroup::getDeviceInfo(int portIndex)
{
    OstProto::PortId *portId;
    OstProto::PortDeviceList *deviceList;
    OstProto::PortNeighborList *neighList;
    PbRpcController *controller;

    Q_ASSERT(portIndex < mPorts.size());

    if (state() != QAbstractSocket::ConnectedState)
        return;

    portId = new OstProto::PortId;
    portId->set_id(mPorts[portIndex]->id());
    deviceList = new OstProto::PortDeviceList;
    controller = new PbRpcController(portId, deviceList);

    serviceStub->getDeviceList(controller, portId, deviceList,
        NewCallback(this, &PortGroup::processDeviceList,
                    portIndex, controller));

    portId = new OstProto::PortId;
    portId->set_id(mPorts[portIndex]->id());
    neighList = new OstProto::PortNeighborList;
    controller = new PbRpcController(portId, neighList);

    serviceStub->getDeviceNeighbors(controller, portId, neighList,
        NewCallback(this, &PortGroup::processDeviceNeighbors,
                    portIndex, controller));
}

void PortGroup::processDeviceList(int portIndex, PbRpcController *controller)
{
    OstProto::PortDeviceList *deviceList
        = static_cast<OstProto::PortDeviceList*>(controller->response());

    qDebug("In %s (portIndex = %d)", __FUNCTION__, portIndex);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (deviceList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            deviceList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    mPorts[portIndex]->clearDeviceList();
    for(int i = 0; i < deviceList->ExtensionSize(OstEmul::device); i++) {
        mPorts[portIndex]->insertDevice(
                deviceList->GetExtension(OstEmul::device, i));
    }

_exit:
    delete controller;
}

void PortGroup::processDeviceNeighbors(
        int portIndex, PbRpcController *controller)
{
    OstProto::PortNeighborList *neighList
        = static_cast<OstProto::PortNeighborList*>(controller->response());

    qDebug("In %s (portIndex = %d)", __FUNCTION__, portIndex);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(),  mPorts[portIndex]->id(),controller->ErrorString());
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (neighList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            neighList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    mPorts[portIndex]->clearDeviceNeighbors();
    for(int i=0; i < neighList->ExtensionSize(OstEmul::device_neighbor); i++) {
        mPorts[portIndex]->insertDeviceNeighbors(
                neighList->GetExtension(OstEmul::device_neighbor, i));
    }

    mPorts[portIndex]->deviceInfoRefreshed();

_exit:
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

    logInfo(id(), mPorts[portIndex]->id(),
            QString("Modifying port configuration"));
    OstProto::Port *port = portConfigList->add_port();
    port->CopyFrom(portConfig);
    port->mutable_port_id()->set_id(mPorts[portIndex]->id());

    PbRpcController *controller = new PbRpcController(portConfigList, ack);
    serviceStub->modifyPort(controller, portConfigList, ack, 
        NewCallback(this, &PortGroup::processModifyPortAck, controller));

    logInfo(id(), mPorts[portIndex]->id(),
            QString("Re-building packets"));
    OstProto::BuildConfig *buildConfig = new OstProto::BuildConfig;
    ack = new OstProto::Ack;
    controller = new PbRpcController(buildConfig, ack);
    buildConfig->mutable_port_id()->set_id(mPorts[portIndex]->id());
    serviceStub->build(controller, buildConfig, ack,
        NewCallback(this, &PortGroup::processModifyPortBuildAck,
                    true, controller));
}

void PortGroup::processModifyPortAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
    }

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));

    delete controller;
}

void PortGroup::processModifyPortBuildAck(bool restoreUi, PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
    }

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));

    if (restoreUi) {
        mainWindow->setEnabled(true);
        QApplication::restoreOverrideCursor();
    }
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
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    for(int i = 0; i < portConfigList->port_size(); i++)
    {
        uint    id;

        id = portConfigList->port(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updatePortConfig(portConfigList->mutable_port(i));

        emit portGroupDataChanged(mPortGroupId, id);
    }


_exit:
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
    const OstProto::PortContent *newPortContent
        = atConnectPortConfig_.at(portIndex);

    qDebug("In %s (portIndex = %d)", __FUNCTION__, portIndex);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__, 
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (streamIdList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            streamIdList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    if (newPortContent)
    {
        // This port needs to configured with new content - to do this
        // we'll insert the following RPC sequence at this point and once
        // this sequence is over, return to the regular RPC sequence by
        // re-requesting getStreamId()
        //   * delete (existing) deviceGroups
        //     (already done by processDeviceIdList)
        //   * delete (existing) streams
        //   * modify port
        //   * add (new) deviceGroup ids
        //   * modify (new) deviceGroups
        //   * add (new) stream ids
        //   * modify (new) streams
        // XXX: This assumes getDeviceGroupIdList() was invoked before
        // getStreamIdList() - if the order changes this code will break!

        // XXX: same name as input param, but shouldn't cause any problem
        PbRpcController *controller;
        quint32 portId = mPorts[portIndex]->id();
        QString myself = appSettings->value(kUserKey, kUserDefaultValue)
                            .toString();

        // delete all existing streams
        if (streamIdList->stream_id_size())
        {
            OstProto::StreamIdList *streamIdList2 = new OstProto::StreamIdList;
            streamIdList2->CopyFrom(*streamIdList);

            OstProto::Ack *ack = new OstProto::Ack;
            controller = new PbRpcController(streamIdList2, ack);

            serviceStub->deleteStream(controller, streamIdList2, ack,
                NewCallback(this, &PortGroup::processDeleteStreamAck,
                            controller));
        }

        OstProto::Port portCfg = newPortContent->port_config();
        if (mPorts[portIndex]->modifiablePortConfig(portCfg))
        {
            OstProto::PortConfigList *portConfigList =
                    new OstProto::PortConfigList;
            OstProto::Port *port = portConfigList->add_port();
            port->CopyFrom(portCfg);
            if (port->has_user_name())
                port->set_user_name(qPrintable(myself)); // overwrite

            OstProto::Ack *ack = new OstProto::Ack;
            controller = new PbRpcController(portConfigList, ack);

            serviceStub->modifyPort(controller, portConfigList, ack,
                NewCallback(this, &PortGroup::processModifyPortAck,
                            controller));
        }

        // add/modify deviceGroups
        if (newPortContent->device_groups_size())
        {
            OstProto::DeviceGroupIdList *deviceGroupIdList
                    = new OstProto::DeviceGroupIdList;
            OstProto::DeviceGroupConfigList *deviceGroupConfigList
                    = new OstProto::DeviceGroupConfigList;
            deviceGroupIdList->mutable_port_id()->set_id(portId);
            deviceGroupConfigList->mutable_port_id()->set_id(portId);
            for (int i = 0; i < newPortContent->device_groups_size(); i++)
            {
                const OstProto::DeviceGroup &dg
                        = newPortContent->device_groups(i);
                deviceGroupIdList->add_device_group_id()->set_id(
                        dg.device_group_id().id());
                deviceGroupConfigList->add_device_group()->CopyFrom(dg);
            }

            OstProto::Ack *ack = new OstProto::Ack;
            controller = new PbRpcController(deviceGroupIdList, ack);

            serviceStub->addDeviceGroup(controller, deviceGroupIdList, ack,
                    NewCallback(this, &PortGroup::processAddDeviceGroupAck,
                                controller));

            ack = new OstProto::Ack;
            controller = new PbRpcController(deviceGroupConfigList, ack);

            serviceStub->modifyDeviceGroup(controller,
                    deviceGroupConfigList, ack,
                    NewCallback(this, &PortGroup::processModifyDeviceGroupAck,
                        portIndex, controller));
        }

        // add/modify streams
        if (newPortContent->streams_size())
        {
            OstProto::StreamIdList *streamIdList = new OstProto::StreamIdList;
            OstProto::StreamConfigList *streamConfigList =
                    new OstProto::StreamConfigList;
            streamIdList->mutable_port_id()->set_id(portId);
            streamConfigList->mutable_port_id()->set_id(portId);
            for (int i = 0; i < newPortContent->streams_size(); i++)
            {
                const OstProto::Stream &s = newPortContent->streams(i);
                streamIdList->add_stream_id()->set_id(s.stream_id().id());
                streamConfigList->add_stream()->CopyFrom(s);
            }

            OstProto::Ack *ack = new OstProto::Ack;
            controller = new PbRpcController(streamIdList, ack);

            serviceStub->addStream(controller, streamIdList, ack,
                    NewCallback(this, &PortGroup::processAddStreamAck,
                                controller));

            ack = new OstProto::Ack;
            controller = new PbRpcController(streamConfigList, ack);

            serviceStub->modifyStream(controller, streamConfigList, ack,
                    NewCallback(this, &PortGroup::processModifyStreamAck,
                        portIndex, controller));
        }

        // build packets using the new config
        OstProto::BuildConfig *buildConfig = new OstProto::BuildConfig;
        OstProto::Ack *ack = new OstProto::Ack;
        controller = new PbRpcController(buildConfig, ack);
        buildConfig->mutable_port_id()->set_id(mPorts[portIndex]->id());
        serviceStub->build(controller, buildConfig, ack,
            NewCallback(this, &PortGroup::processModifyPortBuildAck,
                        false, controller));

        // delete newPortConfig
        atConnectPortConfig_[portIndex] = NULL;

        // return to normal sequence re-starting from
        // getDeviceGroupIdList() and getStreamIdList()
        OstProto::PortId *portId2 = new OstProto::PortId;
        portId2->set_id(portId);

        OstProto::DeviceGroupIdList *devGrpIdList
                = new OstProto::DeviceGroupIdList;
        controller = new PbRpcController(portId2, devGrpIdList);

        serviceStub->getDeviceGroupIdList(controller, portId2, devGrpIdList,
                NewCallback(this, &PortGroup::processDeviceGroupIdList,
                    portIndex, controller));

        portId2 = new OstProto::PortId;
        portId2->set_id(portId);
        OstProto::StreamIdList *streamIdList = new OstProto::StreamIdList;
        controller = new PbRpcController(portId2, streamIdList);

        serviceStub->getStreamIdList(controller, portId2, streamIdList,
                NewCallback(this, &PortGroup::processStreamIdList,
                    portIndex, controller));
    }
    else
    {
        for(int i = 0; i < streamIdList->stream_id_size(); i++)
        {
            uint streamId;

            streamId = streamIdList->stream_id(i).id();
            mPorts[portIndex]->insertStream(streamId);
        }

        mPorts[portIndex]->when_syncComplete();

        getStreamConfigList(portIndex);
    }

_exit:
    delete controller;
}

void PortGroup::getStreamConfigList(int portIndex)
{
    if (mPorts[portIndex]->numStreams() == 0)
        return;

    qDebug("requesting stream config list (port %d)...", portIndex);

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
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
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

#if 0
    // FIXME: incorrect check - will never be true if last port does not have any streams configured
    // Are we done for all ports?
    if (portIndex >= (numPorts()-1))
    {
        // FIXME(HI): some way to reset streammodel 
    }
#endif

_exit:
    delete controller;
}

void PortGroup::getDeviceGroupIdList()
{
    using OstProto::PortId;
    using OstProto::DeviceGroupIdList;

    for (int portIndex = 0; portIndex < numPorts(); portIndex++)
    {
        PortId *portId = new PortId;
        DeviceGroupIdList *devGrpIdList = new DeviceGroupIdList;
        PbRpcController *controller = new PbRpcController(portId, devGrpIdList);

        portId->set_id(mPorts[portIndex]->id());

        serviceStub->getDeviceGroupIdList(controller, portId, devGrpIdList,
                NewCallback(this, &PortGroup::processDeviceGroupIdList,
                    portIndex, controller));
    }
}

void PortGroup::processDeviceGroupIdList(
        int portIndex,
        PbRpcController *controller)
{
    using OstProto::DeviceGroupIdList;

    DeviceGroupIdList *devGrpIdList = static_cast<DeviceGroupIdList*>(
                                                controller->response());
    const OstProto::PortContent *newPortContent = atConnectPortConfig_.at(
                                                                    portIndex);

    qDebug("In %s (portIndex = %d)", __FUNCTION__, portIndex);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (devGrpIdList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            devGrpIdList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    if (newPortContent)
    {
        // We delete all existing deviceGroups
        // Remaining stuff is done in processStreamIdList() - see notes there
        if (devGrpIdList->device_group_id_size())
        {
            OstProto::DeviceGroupIdList *devGrpIdList2
                    = new OstProto::DeviceGroupIdList;
            devGrpIdList2->CopyFrom(*devGrpIdList);

            OstProto::Ack *ack = new OstProto::Ack;
            PbRpcController *controller
                    = new PbRpcController(devGrpIdList2, ack);

            serviceStub->deleteDeviceGroup(controller, devGrpIdList2, ack,
                NewCallback(this, &PortGroup::processDeleteDeviceGroupAck,
                            controller));
        }
    }
    else
    {
        for(int i = 0; i < devGrpIdList->device_group_id_size(); i++)
        {
            uint devGrpId;

            devGrpId = devGrpIdList->device_group_id(i).id();
            mPorts[portIndex]->insertDeviceGroup(devGrpId);
        }

        getDeviceGroupConfigList(portIndex);
    }

_exit:
    delete controller;
}

void PortGroup::getDeviceGroupConfigList(int portIndex)
{
    using OstProto::DeviceGroupId;
    using OstProto::DeviceGroupIdList;
    using OstProto::DeviceGroupConfigList;

    if (mPorts[portIndex]->numDeviceGroups() == 0) {
        // No devGrps but we may still have devices (hostDev)
        getDeviceInfo(portIndex);
        return;
    }

    qDebug("requesting device group config list (port %d) ...", portIndex);

    DeviceGroupIdList *devGrpIdList = new DeviceGroupIdList;
    DeviceGroupConfigList *devGrpCfgList = new DeviceGroupConfigList;
    PbRpcController *controller = new PbRpcController(
            devGrpIdList, devGrpCfgList);

    devGrpIdList->mutable_port_id()->set_id(mPorts[portIndex]->id());
    for (int j = 0; j < mPorts[portIndex]->numDeviceGroups(); j++)
    {
        DeviceGroupId *dgid = devGrpIdList->add_device_group_id();
        dgid->set_id(mPorts[portIndex]->deviceGroupByIndex(j)
                                                ->device_group_id().id());
    }

    serviceStub->getDeviceGroupConfig(controller,
            devGrpIdList, devGrpCfgList,
            NewCallback(this, &PortGroup::processDeviceGroupConfigList,
                portIndex, controller));
}

void PortGroup::processDeviceGroupConfigList(int portIndex,
        PbRpcController *controller)
{
    using OstProto::DeviceGroupConfigList;

    DeviceGroupConfigList *devGrpCfgList =
        static_cast<OstProto::DeviceGroupConfigList*>(controller->response());

    qDebug("In %s", __PRETTY_FUNCTION__);

    Q_ASSERT(portIndex < numPorts());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), mPorts[portIndex]->id(), controller->ErrorString());
        goto _exit;
    }

    Q_ASSERT(portIndex < numPorts());

    if (devGrpCfgList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("Invalid portId %d (expected %d) received for portIndex %d",
            devGrpCfgList->port_id().id(), mPorts[portIndex]->id(), portIndex);
        goto _exit;
    }

    for(int i = 0; i <  devGrpCfgList->device_group_size(); i++)
    {
        uint dgid = devGrpCfgList->device_group(i).device_group_id().id();

        mPorts[portIndex]->updateDeviceGroup(dgid,
                                    devGrpCfgList->mutable_device_group(i));
    }

    getDeviceInfo(portIndex);

#if 0
    // FIXME: incorrect check - will never be true if last port does not have any deviceGroups configured
    // Are we done for all ports?
    if (portIndex >= (numPorts()-1))
    {
        // FIXME: reset deviceGroupModel?
    }
#endif

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

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));

_exit:
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

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));

_exit:
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

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }
    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
_exit:
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

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }
    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
_exit:
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
        qDebug("Temp CapFile = %s", qPrintable(capFile->fileName()));
        controller->setBinaryBlob(capFile);

        serviceStub->getCaptureBuffer(controller, portId, buf,
            NewCallback(this, &PortGroup::processViewCaptureAck, controller));
    }
_exit:
    return;
}

void PortGroup::processViewCaptureAck(PbRpcController *controller)
{
    OstProto::PortId *portId = static_cast<OstProto::PortId*>(
                                                controller->request());
    QFile *capFile = static_cast<QFile*>(controller->binaryBlob());

    QString viewer = appSettings->value(kWiresharkPathKey, 
            kWiresharkPathDefaultValue).toString();

    qDebug("In %s", __FUNCTION__);

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), portId->id(), controller->ErrorString());
        goto _exit;
    }

    capFile->flush();
    capFile->close();

    if (!QFile::exists(viewer))
    {
        logError(QString("Wireshark does not exist at %1").arg(viewer));
        QMessageBox::warning(NULL, "Can't find Wireshark", 
                viewer + QString(" does not exist!\n\nPlease correct the path"
                " to Wireshark in the Preferences."));
        goto _exit;
    }

    if (!QProcess::startDetached(viewer, QStringList() << capFile->fileName())) {
        qDebug("Failed starting Wireshark");
        logError(QString("Failed to start %1").arg(viewer));
    }

_exit:
    delete controller;
}

void PortGroup::resolveDeviceNeighbors(QList<uint> *portList)
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

        serviceStub->resolveDeviceNeighbors(controller, portIdList, ack,
            NewCallback(this, &PortGroup::processResolveDeviceNeighborsAck,
                        controller));
    }
_exit:
    return;
}

void PortGroup::processResolveDeviceNeighborsAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
_exit:
    delete controller;
}

void PortGroup::clearDeviceNeighbors(QList<uint> *portList)
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

        serviceStub->clearDeviceNeighbors(controller, portIdList, ack,
            NewCallback(this, &PortGroup::processClearDeviceNeighborsAck,
                        controller));
    }
_exit:
    return;
}

void PortGroup::processClearDeviceNeighborsAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
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
        logError(id(), statsController->ErrorString());
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
            NewCallback(this, &PortGroup::processClearPortStatsAck,
                        controller));
    }
_exit:
    return;
}

void PortGroup::processClearPortStatsAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);
    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());

    // Refresh stats immediately after a stats clear/reset
    getPortStats();

    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }

    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
_exit:
    delete controller;
}

bool PortGroup::clearStreamStats(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return false;

    OstProto::StreamGuidList *guidList = new OstProto::StreamGuidList;
    OstProto::Ack *ack = new OstProto::Ack;
    PbRpcController *controller = new PbRpcController(guidList, ack);

    if (portList == NULL)
        guidList->mutable_port_id_list()->CopyFrom(*portIdList_);
    else
        for (int i = 0; i < portList->size(); i++)
            guidList->mutable_port_id_list()->add_port_id()
                ->set_id(portList->at(i));

    serviceStub->clearStreamStats(controller, guidList, ack,
            NewCallback(this, &PortGroup::processClearStreamStatsAck,
                        controller));

    return true;
}

void PortGroup::processClearStreamStatsAck(PbRpcController *controller)
{
    qDebug("In %s", __FUNCTION__);

    OstProto::Ack *ack = static_cast<OstProto::Ack*>(controller->response());
    if (controller->Failed())
    {
        qDebug("%s: rpc failed(%s)", __FUNCTION__,
                qPrintable(controller->ErrorString()));
        logError(id(), controller->ErrorString());
        goto _exit;
    }
    if (ack->status())
        logError(id(), QString::fromStdString(ack->notes()));
_exit:
    delete controller;
}

bool PortGroup::getStreamStats(QList<uint> *portList)
{
    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return false;

    OstProto::StreamGuidList *guidList = new OstProto::StreamGuidList;
    OstProto::StreamStatsList *statsList = new OstProto::StreamStatsList;
    PbRpcController *controller = new PbRpcController(guidList, statsList);

    if (portList == NULL)
        guidList->mutable_port_id_list()->CopyFrom(*portIdList_);
    else
        for (int i = 0; i < portList->size(); i++)
            guidList->mutable_port_id_list()->add_port_id()
                ->set_id(portList->at(i));

    serviceStub->getStreamStats(controller, guidList, statsList,
            NewCallback(this, &PortGroup::processStreamStatsList, controller));

    return true;
}

void PortGroup::processStreamStatsList(PbRpcController *controller)
{
    using OstProto::StreamStatsList;

    qDebug("In %s", __FUNCTION__);

    StreamStatsList *streamStatsList =
        static_cast<StreamStatsList*>(controller->response());

    // XXX: It is required to emit the signal even if the returned
    // streamStatsList contains no records since the recipient
    // StreamStatsModel slot needs to disconnect this signal-slot
    // connection to prevent future stream stats for this portgroup
    // to be sent to it
    emit streamStatsReceived(mPortGroupId, streamStatsList);

    delete controller;
}

