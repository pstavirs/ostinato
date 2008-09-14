#ifndef _MY_SERVICE_H
#define _MY_SERVICE_H


#if 0
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#endif

#include "../common/protocol.pb.h"
#include "abstracthost.h"
#include <pcap.h>
#include <QtGlobal>
#include <QList>
#include <QThread>

#include "../rpc/pbhelper.h"

#define MAX_PKT_HDR_SIZE			1536
#define MAX_STREAM_NAME_SIZE		64

class MyService;

class StreamInfo
{
	friend class MyService;
	friend class PortInfo;

	OstProto::Stream	d;

	StreamInfo() { PbHelper pbh; pbh.ForceSetSingularDefault(&d); }
	int StreamInfo::makePacket(uchar *buf, int bufMaxSize);
};


class PortInfo
{
	friend class MyService;

	class PortMonitor: public QThread
	{
		friend class PortInfo;

		PortInfo	*port;
	public:
		PortMonitor(PortInfo *port);
		static void callback(u_char *state, 
			const struct pcap_pkthdr *header, const u_char *pkt_data);
		void run();
	};

	OstProto::Port			d;

	pcap_if_t				*dev;
	pcap_t					*devHandle;
	pcap_send_queue			*sendQueue;
	bool					isSendQueueDirty;
	PortMonitor				monitor;

	struct PortStats
	{
		quint64	rxPkts;
		quint64	rxBytes;
		quint64	rxPps;
		quint64	rxBps;

		quint64	txPkts;
		quint64	txBytes;
		quint64	txPps;
		quint64	txBps;
	};

	struct PortStats		stats;

	/*! StreamInfo::d::stream_id and index into streamList[] are NOT same! */
	QList<StreamInfo>		streamList;

public:
	PortInfo::PortInfo(uint id, pcap_if_t *dev);
	uint id() { return d.port_id().id(); }
	bool isDirty() { return isSendQueueDirty; }
	void setDirty(bool dirty) { isSendQueueDirty = dirty; }
	void update();
	void startTransmit();
	void stopTransmit();
	void resetStats();
};


class MyService: public OstProto::OstService
{
	AbstractHost	*host;
	char			logStr[1024];

	uint		 		numPorts;

	/*! PortInfo::d::port_id and index into portInfo[] are same! */
	QList<PortInfo*>	portInfo;
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
