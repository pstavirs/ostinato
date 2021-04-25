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

#include "../common/protocol.pb.h"
#include "../rpc/sharedprotobufmessage.h"

#include <QList>
#include <QObject>
#include <QReadWriteLock>

#define MAX_PKT_HDR_SIZE            1536
#define MAX_STREAM_NAME_SIZE        64

class AbstractPort;

class MyService: public QObject, public OstProto::OstService
{
    Q_OBJECT
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
    virtual void startTransmit(::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void stopTransmit(::google::protobuf::RpcController* controller,
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

    virtual void getStreamStats(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamGuidList* request,
        ::OstProto::StreamStatsList* response,
        ::google::protobuf::Closure* done);
    virtual void clearStreamStats(::google::protobuf::RpcController* controller,
        const ::OstProto::StreamGuidList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);

    virtual void checkVersion(::google::protobuf::RpcController* controller,
        const ::OstProto::VersionInfo* request,
        ::OstProto::VersionCompatibility* response,
        ::google::protobuf::Closure* done);

    virtual void build(::google::protobuf::RpcController* controller,
        const ::OstProto::BuildConfig* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);

    // DeviceGroup and Protocol Emulation
    virtual void getDeviceGroupIdList(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::PortId* request,
        ::OstProto::DeviceGroupIdList* response,
        ::google::protobuf::Closure* done);
    virtual void getDeviceGroupConfig(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::DeviceGroupIdList* request,
        ::OstProto::DeviceGroupConfigList* response,
        ::google::protobuf::Closure* done);
    virtual void addDeviceGroup(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::DeviceGroupIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void deleteDeviceGroup(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::DeviceGroupIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void modifyDeviceGroup(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::DeviceGroupConfigList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);

    virtual void getDeviceList(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::PortId* request,
        ::OstProto::PortDeviceList* response,
        ::google::protobuf::Closure* done);

    virtual void resolveDeviceNeighbors(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void clearDeviceNeighbors(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::PortIdList* request,
        ::OstProto::Ack* response,
        ::google::protobuf::Closure* done);
    virtual void getDeviceNeighbors(
        ::google::protobuf::RpcController* controller,
        const ::OstProto::PortId* request,
        ::OstProto::PortNeighborList* response,
        ::google::protobuf::Closure* done);

    friend quint64 getDeviceMacAddress(
            int portId, int streamId, int frameIndex);
    friend quint64 getNeighborMacAddress(
            int portId, int streamId, int frameIndex);
signals:
    void notification(int notifType, SharedProtobufMessage notifData);

private:
    QString frameValueErrorNotes(int portId, int error);

    /* 
     * NOTES:
     * - AbstractPort::id() and index into portInfo[] are same!
     * - portLock[] size and order should be same as portInfo[] as the
     *   same index is used for both.
     * - we assume that once populated by the constructor, the list(s) 
     *   never change (objects in the list can change, but not the list itself)
     * - locking is at port granularity, not at stream granularity - for now
     *   this seems sufficient. Revisit later, if required
     */
    QList<AbstractPort*>    portInfo;
    QList<QReadWriteLock*>  portLock;

};

#endif
