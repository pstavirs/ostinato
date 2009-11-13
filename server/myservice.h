#ifndef _MY_SERVICE_H
#define _MY_SERVICE_H


#if 0
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#endif

#include "../common/protocol.pb.h"
#include "../common/streambase.h"
#include "abstracthost.h"
#include <pcap.h>
#include <QtGlobal>
#include <QList>
#include <QThread>
#include <QTemporaryFile>

#include "../rpc/pbhelper.h"
#include "pcapextra.h"

#ifdef Q_OS_WIN32
#include <packet32.h>
#endif

#ifdef Q_OS_WIN32
#define OID_GEN_MEDIA_CONNECT_STATUS	0x00010114
#endif

#define MAX_PKT_HDR_SIZE			1536
#define MAX_STREAM_NAME_SIZE		64

//! 7 byte Preamble + 1 byte SFD + 4 byte FCS
#define ETH_FRAME_HDR_SIZE			12


class MyService;

class StreamInfo : public StreamBase
{
	friend class MyService;
	friend class PortInfo;

	OstProto::StreamId			mStreamId;

public:
	StreamInfo();
	~StreamInfo();
};


class PortInfo
{
	friend class MyService;

	class PortMonitorRx: public QThread
	{
		friend class PortInfo;

		PortInfo			*port;
#ifdef Q_OS_WIN32
		PPACKET_OID_DATA	oidData;
#endif
	public:
		PortMonitorRx(PortInfo *port);
		static void callbackRx(u_char *state, 
			const struct pcap_pkthdr *header, const u_char *pkt_data);
		void run();
	};

	class PortMonitorTx: public QThread
	{
		friend class PortInfo;

		PortInfo			*port;
#ifdef Q_OS_WIN32
		PPACKET_OID_DATA	oidData;
#endif
	public:
		PortMonitorTx(PortInfo *port);
		static void callbackTx(u_char *state, 
			const struct pcap_pkthdr *header, const u_char *pkt_data);
		void run();
	};

	class PortTransmitter: public QThread
	{
		friend class PortInfo;

		PortInfo	*port;
		int			m_stop;

	public:
		PortTransmitter(PortInfo *port);
		void run();
		void stop();
	};

	class PortCapture: public QThread
	{
		friend class PortInfo;

		PortInfo		*port;
		int				m_stop;
		pcap_t			*capHandle;
		pcap_dumper_t	*dumpHandle;
		QTemporaryFile	capFile;

	public:
		PortCapture(PortInfo *port);
		~PortCapture();
		void run();
		void stop();
		QFile* captureFile();
	};

#ifdef Q_OS_WIN32
	LPADAPTER			adapter;
	PPACKET_OID_DATA	oidData;
#endif

	OstProto::Port			d;
	OstProto::LinkState		linkState;

	struct PortStats
	{
		quint64	rxPkts;
		quint64	rxBytes;
		quint64	rxPktsNic;
		quint64	rxBytesNic;
		quint64	rxPps;
		quint64	rxBps;

		quint64	txPkts;
		quint64	txBytes;
		quint64	txPktsNic;
		quint64	txBytesNic;
		quint64	txPps;
		quint64	txBps;
	};

	//! \todo Need lock for stats access/update


	//! Stuff we need to maintain since PCAP doesn't as of now. As and when
	// PCAP supports it, we'll remove from here
	struct PcapExtra
	{

		//! PCAP doesn't do any tx stats
		quint64		txPkts;
		quint64		txBytes;

	};

	pcap_if_t				*dev;
	pcap_t					*devHandleRx;
	pcap_t					*devHandleTx;
	QList<ost_pcap_send_queue> sendQueueList;
	int						returnToQIdx; // FIXME(MED): combine with sendQList
	bool					isSendQueueDirty;
	PcapExtra				pcapExtra;
	PortMonitorRx			monitorRx;
	PortMonitorTx			monitorTx;
	PortTransmitter			transmitter;
	PortCapture				capturer;

	struct PortStats		epochStats;
	struct PortStats		stats;
	struct timeval			lastTsRx;	//! used for Rate Stats calculations
	struct timeval			lastTsTx;	//! used for Rate Stats calculations

	/*! StreamInfo::d::stream_id and index into streamList[] are NOT same! */
	QList<StreamInfo*>		streamList;

public:
	PortInfo(uint id, pcap_if_t *dev);
	uint id() { return d.port_id().id(); }
	void updateLinkState();
	bool isDirty() { return isSendQueueDirty; }
	void setDirty(bool dirty) { isSendQueueDirty = dirty; }
	void update();
	void startTransmit();
	void stopTransmit();
	void startCapture();
	void stopCapture();
	QFile* captureFile();
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
};

#endif
