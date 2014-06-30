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


#include "myservice.h"

#if 0
#include <qglobal.h>
#include <qendian.h>
#include "qdebug.h"

#include "../common/protocollistiterator.h"
#include "../common/abstractprotocol.h"
#endif

#include "../common/streambase.h"
#include "../rpc/pbrpccontroller.h"
#include "portmanager.h"

MyService::MyService()
{
    PortManager *portManager = PortManager::instance();
    int n = portManager->portCount();

    for (int i = 0; i < n; i++) {
        portInfo.append(portManager->port(i));
        portLock.append(new QReadWriteLock());
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
    }

    done->Run();
}

void MyService::modifyPort(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortConfigList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_size(); i++)
    {
        OstProto::Port port;
        int id;

        port = request->port(i);
        id = port.port_id().id();
        if (id < portInfo.size())
        {
            portLock[id]->lockForWrite();
            portInfo[id]->modify(port);
            portLock[id]->unlock();
        }
    }

    //! \todo (LOW): fill-in response "Ack"????
    done->Run();
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
    controller->SetFailed("Invalid Port Id");
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
            continue;        //! \todo(LOW): Partial status of RPC

        s = response->add_stream();
        stream->protoDataCopyInto(*s);
    }
    portLock[portId]->unlock();

    done->Run();
    return;

_invalid_port:
    controller->SetFailed("invalid portid");
    done->Run();
}

void MyService::addStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
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
        if (stream)
            continue;        //! \todo (LOW): Partial status of RPC

        // Append a new "default" stream - actual contents of the new stream is
        // expected in a subsequent "modifyStream" request - set the stream id
        // now itself however!!!
        stream = new StreamBase;
        stream->setId(request->stream_id(i).id());
        portInfo[portId]->addStream(stream);
    }
    portLock[portId]->unlock();

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
    return;

_port_busy:
    controller->SetFailed("Port Busy");
    goto _exit;

_invalid_port:
    controller->SetFailed("invalid portid");
_exit:
    done->Run();
}

void MyService::deleteStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    int portId;

    qDebug("In %s", __PRETTY_FUNCTION__);

    portId = request->port_id().id();
    if ((portId < 0) || (portId >= portInfo.size()))
        goto _invalid_port;

    if (portInfo[portId]->isTransmitOn())
        goto _port_busy;

    portLock[portId]->lockForWrite();
    for (int i = 0; i < request->stream_id_size(); i++)
        portInfo[portId]->deleteStream(request->stream_id(i).id());
    portLock[portId]->unlock();

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
    return;

_port_busy:
    controller->SetFailed("Port Busy");
    goto _exit;
_invalid_port:
    controller->SetFailed("invalid portid");
_exit:
    done->Run();
}

void MyService::modifyStream(::google::protobuf::RpcController* controller,
    const ::OstProto::StreamConfigList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
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
        StreamBase    *stream;

        stream = portInfo[portId]->stream(request->stream(i).stream_id().id());
        if (stream)
        {
            stream->protoDataCopyFrom(request->stream(i));
            portInfo[portId]->setDirty();
        }
    }

    if (portInfo[portId]->isDirty())
        portInfo[portId]->updatePacketList();
    portLock[portId]->unlock();

    //! \todo(LOW): fill-in response "Ack"????

    done->Run();
    return;

_port_busy:
    controller->SetFailed("Port Busy");
    goto _exit;
_invalid_port:
    controller->SetFailed("invalid portid");
_exit:
    done->Run();
}

void MyService::startTransmit(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue;     //! \todo (LOW): partial RPC?

        portLock[portId]->lockForWrite();
        portInfo[portId]->startTransmit();
        portLock[portId]->unlock();
    }

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
}

void MyService::stopTransmit(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue;     //! \todo (LOW): partial RPC?

        portLock[portId]->lockForWrite();
        portInfo[portId]->stopTransmit();
        portLock[portId]->unlock();
    }

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
}

void MyService::startCapture(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue;     //! \todo (LOW): partial RPC?

        portLock[portId]->lockForWrite();
        portInfo[portId]->startCapture();
        portLock[portId]->unlock();
    }

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
}

void MyService::stopCapture(::google::protobuf::RpcController* /*controller*/,
    const ::OstProto::PortIdList* request,
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);
    for (int i=0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue;     //! \todo (LOW): partial RPC?

        portLock[portId]->lockForWrite();
        portInfo[portId]->stopCapture();
        portLock[portId]->unlock();
    }

    //! \todo (LOW): fill-in response "Ack"????

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
            continue;     //! \todo(LOW): partial rpc?

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
    ::OstProto::Ack* /*response*/,
    ::google::protobuf::Closure* done)
{
    qDebug("In %s", __PRETTY_FUNCTION__);

    for (int i = 0; i < request->port_id_size(); i++)
    {
        int portId;

        portId = request->port_id(i).id();
        if ((portId < 0) || (portId >= portInfo.size()))
            continue;     //! \todo (LOW): partial RPC?

        portLock[portId]->lockForWrite();
        portInfo[portId]->resetStats();
        portLock[portId]->unlock();
    }

    //! \todo (LOW): fill-in response "Ack"????

    done->Run();
}
