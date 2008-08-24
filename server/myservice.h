#ifndef _MY_SERVICE_H
#define _MY_SERVICE_H


#if 0
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#endif

#include "../common/protocol.pb.h"
#include "abstracthost.h"
#include <pcap.h>
#include <QList>

#include "../rpc/pbhelper.h"

#define MAX_PKT_HDR_SIZE			1536
#define MAX_STREAM_NAME_SIZE		64

class MyService;

class StreamInfo
{
	friend class MyService;

	OstProto::Stream	d;

	StreamInfo() { PbHelper pbh; pbh.ForceSetSingularDefault(&d); }
};


class PortInfo
{
	friend class MyService;

	OstProto::Port			d;
	pcap_if_t				*dev;

	/*! StreamInfo::d::stream_id and index into streamList[] are NOT same! */
	QList<StreamInfo>		streamList;

public:
	// TODO(LOW): Both setId and setPcapDev() should together form the ctor
	void setId(unsigned int id) { d.mutable_port_id()->set_id(id); }
	void setPcapDev(pcap_if_t	*dev)
	{
		this->dev = dev;
		d.set_name("eth"); // FIXME(MED): suffix portid
		d.set_description(dev->description);
		d.set_is_enabled(true);	// FIXME(MED):check
		d.set_is_oper_up(true); // FIXME(MED):check
		d.set_is_exclusive_control(false); // FIXME(MED): check
	}
};

class MyService: public OstProto::OstService
{
	AbstractHost	*host;
	char			logStr[1024];

	uint		 		numPorts;
	/*! PortInfo::d::port_id and index into portInfo[] are same! */
	PortInfo		*portInfo;
	pcap_if_t		*alldevs;

	int getStreamIndex(unsigned int portIdx,unsigned int streamId);

public:
	MyService(AbstractHost* host);
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
		const ::OstProto::PortIdList* request,
		::OstProto::CaptureBufferList* response,
		::google::protobuf::Closure* done);
	virtual void getStats(::google::protobuf::RpcController* controller,
		const ::OstProto::PortIdList* request,
		::OstProto::PortStatsList* response,
		::google::protobuf::Closure* done);
	virtual void clearStats(::google::protobuf::RpcController* controller,
		const ::OstProto::PortIdList* request,
		::OstProto::Ack* response,
		::google::protobuf::Closure* done);
};

#endif
