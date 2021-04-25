/*
Copyright (C) 2010-2016 Srivats P.

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


#include "myservice.h"

#include "drone.h"

#if 0
#include <qglobal.h>
#include <qendian.h>
#include "qdebug.h"

#include "../common/protocollistiterator.h"
#include "../common/abstractprotocol.h"
#endif

#include "../common/framevalueattrib.h"
#include "../common/streambase.h"
#include "../rpc/pbrpccontroller.h"
#include "device.h"
#include "devicemanager.h"
#include "portmanager.h"

#include <QStringList>


extern Drone *drone;
extern char *version;

MyService::MyService()
{
    PortManager *portManager = PortManager::instance();
    int n = portManager->portCount();

    for (int i = 0; i < n; i++) {
        portInfo.append(portManager->port(i));
#if QT_VERSION >= 0x040400
        portLock.append(new QReadWriteLock(QReadWriteLock::Recursive));
#else
        portLock.append(new QReadWriteLock());
#endif
    }
}

MyService::~MyService()
{
    while (!portLock.isEmpty())
        delete portLock.takeFirst();
    //! \todo Use a singleton destroyer instead 
    // http://www.research.ibm.com/designpatterns/pubs/ph-jun96.txt
    delete PortManager::instance();
}

void MyService::getPortIdList(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::Void* /*request*/,
    ::OstProto::PortIdList* response,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    // No locks are needed here because the list does not change
    // and neither does the port_id

    for (int i = 0; i < portInfo.size(); i++)
    {
        ::OstProto::PortId    *p;

        p = response->add_port_id();
        p->set_id(portInfo[i]->id());
    }

    done->Run();
}

void MyService::getPortConfig(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::PortConfigList* response,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int id;

        id = request->port_id(i).id();
        if (id < portInfo.size())
        {
            OstProto::Port    *p;

            p = response->add_port();
            portLock[id]->lockForRead();
            portInfo[id]->protoDataCopyInto(p);
            portLock[id]->unlock();
        }
        // XXX: no way to inform RPC caller of an invalid port!
    }

    done->Run();
}

void MyService::modifyPort(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortConfigList* request,
    ::OstProto::Ack *response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    // notification needs to be on heap because signal/slot is across threads!
    OstProto::Notification *notif = new OstProto::Notification;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_size(); i++)
    {
        OstProto::Port port;
        int id;

        port = request->port(i);
        id = port.port_id().id();
        if (id < portInfo.size())
        {
            bool dirty;

            if (!portInfo[id]->canModify(port, &dirty)) {
                qDebug("Port %d cannot be modified - stop tx and retry", id);
                error = true;
                notes += QString("Port %1 modify: operation disallowed on "
                                 "transmitting port\n").arg(id);
                continue;
            }

            portLock[id]->lockForWrite();
            portInfo[id]->modify(port);
            portLock[id]->unlock();
            notif->mutable_port_id_list()->add_port_id()->set_id(id);
        }
        else {
            error = true;
            notes += QString("Port %1 modify: invalid port\n").arg(id);
        }
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();

    if (notif->port_id_list().port_id_size()) {
        notif->set_notif_type(OstProto::portConfigChanged);
        emit notification(notif->notif_type(), SharedProtobufMessage(notif));
    }
    else
        delete notif;
}

void MyService::getStreamIdList(::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::StreamIdList* response,
    ::google::protobuf::Closure* done)
{
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    for (int i = 0; i < portInfo[portId]->streamCount(); i++)
    {
        OstProto::StreamId    *s;

        s = response->add_stream_id();
        s->set_id(portInfo[portId]->streamAtIndex(i)->id());
    }
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get stream id list: invalid port")
                            .arg(portId).toStdString());
    done->Run();
}

void MyService::getStreamConfig(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::StreamConfigList* response,
    ::google::protobuf::Closure* done)
{
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    for (int i = 0; i < request->stream_id_size(); i++)
    {
        StreamBase          *stream;
        OstProto::Stream    *s;

        stream = portInfo[portId]->stream(request->stream_id(i).id());
        if (!stream)
            continue;        //! XXX: no way to inform RPC caller of invalid stream id

        s = response->add_stream();
        stream->protoDataCopyInto(*s);
    }
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get stream config: invalid port")
                            .arg(portId).toStdString());
    done->Run();
}

void MyService::addStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->stream_id_size(); i++)
    {
        StreamBase    *stream;

        // If stream with same id as in request exists already ==> error!!
        stream = portInfo[portId]->stream(request->stream_id(i).id());
        if (stream) {
            error = true;
            notes += QString("Port %1 Stream %2 add stream: "
                             "stream already exists\n")
                        .arg(portId).arg(request->stream_id(i).id());
            continue;
        }

        // Append a new "default" stream - actual contents of the new stream is
        // expected in a subsequent "modifyStream" request - set the stream id
        // now itself however!!!
        stream = new StreamBase(portId);
        stream->setId(request->stream_id(i).id());
        portInfo[portId]->addStream(stream);
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 add stream: operation disallowed on "
                                  "transmitting port")
                            .arg(portId).toStdString());
    goto _exit;

_invalid_port:
    controller->SetFailed(QString("Port %1 add stream: invalid port")
                            .arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::deleteStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->stream_id_size(); i++) {
        if (!portInfo[portId]->deleteStream(request->stream_id(i).id())) {
            error = true;
            notes += QString("Port %1 Stream %2 stream delete: "
                             "stream not found\n")
                        .arg(portId).arg(request->stream_id(i).id());
        }
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 delete stream: operation disallowed "                                  "on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;
_invalid_port:
    controller->SetFailed(QString("Port %1 delete stream: invalid port")
                            .arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::modifyStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamConfigList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    int    portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->stream_size(); i++)
    {
        quint32 sid = request->stream(i).stream_id().id();
        StreamBase *stream = portInfo[portId]->stream(sid);
        if (stream)
        {
            stream->protoDataCopyFrom(request->stream(i));
            portInfo[portId]->setDirty();
        }
        else {
            error = true;
            notes += QString("Port %1 Stream %2 modify stream: "
                             "stream not found\n").arg(portId).arg(sid);
        }
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 modify stream: operation disallowed "                                  "on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;
_invalid_port:
    controller->SetFailed(QString("Port %1 modify stream: invalid port")
                            .arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::startTransmit(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;
        int frameError = 0;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 start transmit: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        if (portInfo[portId]->isDirty())
            frameError = portInfo[portId]->updatePacketList();
        portInfo[portId]->startTransmit();
        portLock[portId]->unlock();
        if (frameError) {
            error = true;
            notes += frameValueErrorNotes(portId, frameError);
        }
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);

    done->Run();
}

void MyService::stopTransmit(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 stop transmit: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->stopTransmit();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::startCapture(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 start capture: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->startCapture();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::stopCapture(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);
    for (int i=0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 stop capture: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->stopCapture();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::getCaptureBuffer(::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::CaptureBuffer* /*response*/,
    ::google::protobuf::Closure* done)
{
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    portLock[portId]->lockForWrite();
    portInfo[portId]->stopCapture();
    static_cast<PbRpcController*>(controller)->setBinaryBlob(
        portInfo[portId]->captureData());
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed("invalid portid");
    controller->SetFailed(QString("Port %1 get capture buffer: invalid port")
                            .arg(portId).toStdString());
    done->Run();
}

void MyService::getStats(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::PortStatsList* response,
    ::google::protobuf::Closure* done)
{
    //qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int     portId;
        AbstractPort::PortStats stats;
        OstProto::PortStats     *s;
        OstProto::PortState     *st;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue; // XXX: no way to inform RPC caller of invalid port

        s = response->add_port_stats();
        s->mutable_port_id()->set_id(request->port_id(i).id());

        st = s->mutable_state(); 
        portLock[portId]->lockForRead();
        st->set_link_state(portInfo[portId]->linkState()); 
        st->set_is_transmit_on(portInfo[portId]->isTransmitOn()); 
        st->set_is_capture_on(portInfo[portId]->isCaptureOn()); 

        portInfo[portId]->stats(&stats);
        portLock[portId]->unlock();

#if 0
        if (portId == 2)
            qDebug(">%llu", stats.rxPkts);
#endif

        s->set_rx_pkts(stats.rxPkts);
        s->set_rx_bytes(stats.rxBytes);
        s->set_rx_pps(stats.rxPps);
        s->set_rx_bps(stats.rxBps);

        s->set_tx_pkts(stats.txPkts);
        s->set_tx_bytes(stats.txBytes);
        s->set_tx_pps(stats.txPps);
        s->set_tx_bps(stats.txBps);

        s->set_rx_drops(stats.rxDrops);
        s->set_rx_errors(stats.rxErrors);
        s->set_rx_fifo_errors(stats.rxFifoErrors);
        s->set_rx_frame_errors(stats.rxFrameErrors);
    }

    done->Run();
}

void MyService::clearStats(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 clear statistics: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->resetStats();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::getStreamStats(
    ::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::StreamStatsList* response,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_list().port_id_size(); i++)
    {
        int portId;

        portId = request->port_id_list().port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue; // XXX: no way to inform RPC caller of invalid port

        portLock[portId]->lockForRead();
        if (request->stream_guid_size())
            for (int j = 0; j < request->stream_guid_size(); j++)
                portInfo[portId]->streamStats(request->stream_guid(j).id(),
                                              response);
        else
            portInfo[portId]->streamStatsAll(response);
        portLock[portId]->unlock();
    }

    done->Run();
}

void MyService::clearStreamStats(
    ::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::StreamGuidList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_list().port_id_size(); i++)
    {
        int portId;

        portId = request->port_id_list().port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 clear stream statistics: invalid port\n")
                        .arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        if (request->stream_guid_size())
            for (int j = 0; j < request->stream_guid_size(); j++)
                portInfo[portId]->resetStreamStats(
                        request->stream_guid(j).id());
        else
            portInfo[portId]->resetStreamStatsAll();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::checkVersion(::google::protobuf::RpcController* controller,
    const ::OstProto::VersionInfo* request,
    ::OstProto::VersionCompatibility* response,
    ::google::protobuf::Closure* done)
{
    QString myVersion(version);
    QString clientVersion;
    QStringList my, client;

    qDebug("In %s", __PRETTY_FUNCTION__);

    my = myVersion.split('.');

    Q_ASSERT(my.size() >= 2);

    clientVersion = QString::fromStdString(request->version());
    client = clientVersion.split('.');

    qDebug("client = %s, my = %s", 
            qPrintable(clientVersion), qPrintable(myVersion));
    
    if (client.size() < 2)
        goto _invalid_version;

    // Compare only major and minor numbers
    if (client[0] == my[0] && client[1] == my[1]) {
        response->set_result(OstProto::VersionCompatibility::kCompatible);
        static_cast<PbRpcController*>(controller)->EnableNotif(
            request->client_name() == "python-ostinato" ? false : true);
    }
    else {
        response->set_result(OstProto::VersionCompatibility::kIncompatible);
        response->set_notes(QString("Drone needs controller version %1.%2.x")
                .arg(my[0], my[1]).toStdString());
        static_cast<PbRpcController*>(controller)->TriggerDisconnect();
    }

    done->Run();
    return;

_invalid_version:
    controller->SetFailed("invalid version information");
    done->Run();
}

void MyService::build(::google::protobuf::RpcController* controller,
    const ::OstProto::BuildConfig* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    QString notes;
    int    portId;
    int    frameError = 0;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    if (portInfo[portId]->isDirty())
        frameError = portInfo[portId]->updatePacketList();
    portLock[portId]->unlock();

    if (frameError) {
        notes += frameValueErrorNotes(portId, frameError);
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 build: operation disallowed "                                  "on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;
_invalid_port:
    controller->SetFailed(QString("Port %1 build: invalid port")
                            .arg(portId).toStdString());
_exit:
    done->Run();
}

/*
 * ===================================================================
 * Device Emulation
 * ===================================================================
 * XXX: Streams and Devices are largely non-overlapping from a RPC
 * point of view but they *do* intersect e.g. when a stream is trying to
 * find its associated device and info from that device such as src/dst
 * mac addresses. For this reason, both set of RPCs share the common
 * port level locking
 * ===================================================================
 */
void MyService::getDeviceGroupIdList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::DeviceGroupIdList* response,
    ::google::protobuf::Closure* done)
{
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    for (int i = 0; i < devMgr->deviceGroupCount(); i++)
    {
        OstProto::DeviceGroupId *dgid;

        dgid = response->add_device_group_id();
        dgid->CopyFrom(devMgr->deviceGroupAtIndex(i)->device_group_id());
    }
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get device group id list: "
                                  "invalid port").arg(portId).toStdString());
    done->Run();
}

void MyService::getDeviceGroupConfig(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::DeviceGroupConfigList* response,
    ::google::protobuf::Closure* done)
{
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    for (int i = 0; i < request->device_group_id_size(); i++)
    {
        const OstProto::DeviceGroup *dg;

        dg = devMgr->deviceGroup(request->device_group_id(i).id());
        if (!dg)
            continue; // XXX: no way to inform RPC caller of invalid dgid

        response->add_device_group()->CopyFrom(*dg);
    }
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get device group config: "
                                  "invalid port").arg(portId).toStdString());
    done->Run();
}

void MyService::addDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->device_group_id_size(); i++)
    {
        quint32 id = request->device_group_id(i).id();
        const OstProto::DeviceGroup *dg = devMgr->deviceGroup(id);

        // If device group with same id as in request exists already ==> error!
        if (dg) {
            error = true;
            notes += QString("Port %1 DeviceGroup %2 add device group: "
                             " device group already exists\n")
                        .arg(portId).arg(id);
            continue;
        }

        devMgr->addDeviceGroup(id);
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 add device group: "
                                  "operation disallowed on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;

_invalid_port:
    controller->SetFailed(QString("Port %1 add device group: "
                                  "invalid port")
                            .arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::deleteDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->device_group_id_size(); i++) {
        quint32 id = request->device_group_id(i).id();
        if (!devMgr->deleteDeviceGroup(id)) {
            error = true;
            notes += QString("Port %1 DeviceGroup %2 delete device group: "
                             "device group not found\n").arg(portId).arg(id);
        }
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 delete device group: "
                                  "operation disallowed on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;
_invalid_port:
    controller->SetFailed(QString("Port %1 delete device group: "
                                  "invalid port").arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::modifyDeviceGroup(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::DeviceGroupConfigList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->device_group_size(); i++) {
        quint32 id = request->device_group(i).device_group_id().id();
        if (!devMgr->modifyDeviceGroup(&request->device_group(i))) {
            error = true;
            notes += QString("Port %1 DeviceGroup %2 modify device group: "
                             "device group not found\n").arg(portId).arg(id);
        }
    }
    portLock[portId]->unlock();

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
    return;

_port_busy:
    controller->SetFailed(QString("Port %1 modify device group: "
                                  "operation disallowed on transmitting port")
                            .arg(portId).toStdString());
    goto _exit;
_invalid_port:
    controller->SetFailed(QString("Port %1 modify device group: "
                                  "invalid port").arg(portId).toStdString());
_exit:
    done->Run();
}

void MyService::getDeviceList(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::PortDeviceList* response,
    ::google::protobuf::Closure* done)
{
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    devMgr->getDeviceList(response);
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get device list: "
                                  "invalid port").arg(portId).toStdString());
    done->Run();
}

void MyService::resolveDeviceNeighbors(
    ::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 resolve device neighbors: "
                             "invalid port\n").arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->resolveDeviceNeighbors();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::clearDeviceNeighbors(
    ::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* response,
    ::google::protobuf::Closure* done)
{
    bool error = false;
    QString notes;

    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size())) {
            error = true;
            notes += QString("Port %1 clear device neighbors: "
                             "invalid port\n").arg(portId);
            continue;
        }

        portLock[portId]->lockForWrite();
        portInfo[portId]->clearDeviceNeighbors();
        portLock[portId]->unlock();
    }

    if (error) {
        response->set_status(OstProto::Ack::kRpcError);
        response->set_notes(notes.toStdString());
    }
    else
        response->set_status(OstProto::Ack::kRpcSuccess);
    done->Run();
}

void MyService::getDeviceNeighbors(
    ::google::protobuf::RpcController* controller,
    const ::OstProto::PortId* request,
    ::OstProto::PortNeighborList* response,
    ::google::protobuf::Closure* done)
{
    DeviceManager *devMgr;
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    devMgr = portInfo[portId]->deviceManager();

    response->mutable_port_id()->set_id(portId);
    portLock[portId]->lockForRead();
    devMgr->getDeviceNeighbors(response);
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed(QString("Port %1 get device neighbors: "
                                  "invalid port").arg(portId).toStdString());
    done->Run();
}

QString MyService::frameValueErrorNotes(int portId, int error)
{
    if (!error)
        return QString();

    QString pfx = QString("Port %1: ").arg(portId);
    auto errorFlags = static_cast<FrameValueAttrib::ErrorFlags>(error);

    // If smac resolve fails, dmac will always fail - so check that first
    // and report only that so as not to confuse users (they may not realize
    // that without a source device, we have no ARP table to lookup for dmac)

    if (errorFlags & FrameValueAttrib::UnresolvedSrcMacError)
        return pfx + "Source mac resolve failed for one or more "
                     "streams - Device matching stream's source IP not found\n";

    if (errorFlags & FrameValueAttrib::UnresolvedDstMacError)
        return pfx + "Destination mac resolve failed for one or more "
                     "streams - possible ARP/ND failure\n";

    return QString();
}

/*
 * ===================================================================
 * Friends
 * TODO: Encap these global functions into a DeviceBroker singleton?
 * ===================================================================
 */
quint64 getDeviceMacAddress(int portId, int streamId, int frameIndex)
{
    MyService *service = drone->rpcService();
    DeviceManager *devMgr = NULL;
    quint64 mac;

    if (!service)
        return 0;

    if ((portId >= 0) && (portId < service->portInfo.size()))
        devMgr = service->portInfo[portId]->deviceManager();

    if (!devMgr || !devMgr->deviceCount())
        return 0;

    /*
     * FIXME: We don't need lockForWrite, only lockForRead here.
     * However, this function is called in the following sequence
     * modifyPort() --> updatePacketList() --> frameValue()
     * where modifyPort has already taken a write lock. Qt allows
     * recursive locks, but not of a different type, so we are
     * forced to use lockForWrite here - till we find a different
     * solution.
     */
    service->portLock[portId]->lockForWrite();
    mac = service->portInfo[portId]->deviceMacAddress(streamId, frameIndex);
    service->portLock[portId]->unlock();

    return mac;
}

quint64 getNeighborMacAddress(int portId, int streamId, int frameIndex)
{
    MyService *service = drone->rpcService();
    DeviceManager *devMgr = NULL;
    quint64 mac;

    if (!service)
        return 0;

    if ((portId >= 0) && (portId < service->portInfo.size()))
        devMgr = service->portInfo[portId]->deviceManager();

    if (!devMgr || !devMgr->deviceCount())
        return 0;

    /*
     * FIXME: We don't need lockForWrite, only lockForRead here.
     * See comment in getDeviceMacAddress() for more
     */
    service->portLock[portId]->lockForWrite();
    mac = service->portInfo[portId]->neighborMacAddress(streamId, frameIndex);
    service->portLock[portId]->unlock();

    return mac;
}
