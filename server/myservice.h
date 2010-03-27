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

#ifndef _MY_SERVICE_H
#define _MY_SERVICE_H

#include <QList>

#include "../common/protocol.pb.h"

#define MAX_PKT_HDR_SIZE            1536
#define MAX_STREAM_NAME_SIZE        64

class AbstractPort;

class MyService: public OstProto::OstService
{
public:
    MyService();
    virtual ~MyService();

    /* Methods provided by the service */
    virtual void getPortIdList(::google::protobuf::RpcController* controller,
        const ::OstProto::Void* request,
        ::OstProto::PortIdList* response,
        ::google::protobuf::Closure* done);
    virtual void getPortConfig(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::PortConfigList* response,
        ::google::protobuf::Closure* done);
    virtual void modifyPort(::google::protobuf::RpcController* /*controller*/,
        const ::OstProto::PortConfigList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void getStreamIdList(::google::protobuf::RpcController* controller,
        const ::OstProto::PortId* request,
        ::OstProto::StreamIdList* response,
        ::google::protobuf::Closure* done);
    virtual void getStreamConfig(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamIdList* request,
        ::OstProto::StreamConfigList* response,
        ::google::protobuf::Closure* done);
    virtual void addStream(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void deleteStream(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void modifyStream(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamConfigList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void startTx(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void stopTx(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void startCapture(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void stopCapture(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void getCaptureBuffer(::google::protobuf::RpcController* controller,
        const ::OstProto::PortId* request,
        ::OstProto::CaptureBuffer* response,
        ::google::protobuf::Closure* done);
    virtual void getStats(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::PortStatsList* response,
        ::google::protobuf::Closure* done);
    virtual void clearStats(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);

private:
    /*! AbstractPort::id() and index into portInfo[] are same! */
    QList<AbstractPort*>    portInfo;

};

#endif
