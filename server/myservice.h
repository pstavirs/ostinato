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

#define MAX_PKT_HDR_SIZE			1536
#define MAX_STREAM_NAME_SIZE		64

class MyService;

class StreamInfo
{
	friend class MyService;

	OstProto::Stream	d;

#if 0 // PB
	unsigned int	id;	

	char			name[MAX_STREAM_NAME_SIZE];
	unsigned char	pktHdr[MAX_PKT_HDR_SIZE];
	unsigned short	hdrLen;
	unsigned short	pktLen;
	unsigned int	dataPattern;
	unsigned int	flags;
#define STREAM_FLAG_MASK_STATUS				0x00000001
#define STREAM_FLAG_VALUE_STATUS_DISABLED	0x00000000
#define STREAM_FLAG_VALUE_STATUS_ENABLED	0x00000001

	struct _Stream	*next;
#endif
};


class PortInfo
{
	friend class MyService;

	OstProto::PortConfig	d;
	pcap_if_t				*dev;

	QList<StreamInfo>		streamList;

#if 0 // PB
	unsigned int	portId;	// FIXME:need?
	Stream			*streamHead;
	Stream			*streamTail;
#endif

public:
	// TODO(LOW): Both setId and setPcapDev() should together form the ctor
	void setId(unsigned int id) { d.set_port_id(id); }
	void setPcapDev(pcap_if_t	*dev)
	{
		this->dev = dev;
		d.set_name("eth"); // FIXME: suffix portid
		d.set_description(dev->description);
		d.set_is_enabled(true);	// FIXME:check
		d.set_is_oper_up(true); // FIXME:check
		d.set_is_exclusive_control(false); // FIXME: check
	}
};

class MyService: public OstProto::OstService
{
	AbstractHost	*host;
	char			logStr[1024];

	unsigned 		numPorts;
	PortInfo		*portInfo;
	pcap_if_t		*alldevs;

public:
	MyService(AbstractHost* host);
	virtual ~MyService();

	//static const ::google::protobuf::ServiceDescriptor* descriptor();

	virtual void getPortIdList(::google::protobuf::RpcController* controller,
	const ::OstProto::Void* request,
	::OstProto::PortIdList* response,
	::google::protobuf::Closure* done);
	virtual void getPortConfig(::google::protobuf::RpcController* controller,
	const ::OstProto::PortIdList* request,
	::OstProto::PortConfigList* response,
	::google::protobuf::Closure* done);
	virtual void getStreamIdList(::google::protobuf::RpcController* controller,
	const ::OstProto::PortIdList* request,
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
