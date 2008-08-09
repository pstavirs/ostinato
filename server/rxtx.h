
File not used anymore

#if 0

#ifndef _RXTX_H
#define _RXTX_H

#include "pcap.h"
#include "abstracthost.h"

#include "../common/protocol.h"

#define GET16(x)	(UINT16)(	\
	  (*((UINT8*)x+0) << 16 )	\
	| (*((UINT8*)x+1)))

#define GET32(x)	(UINT32)(	\
	  (*((UINT8*)x+0) << 24)	\
	| (*((UINT8*)x+1) << 16)	\
	| (*((UINT8*)x+2) << 8 )	\
	| (*((UINT8*)x+3)))

#define MAX_PKT_HDR_SIZE			1536
#define MAX_STREAM_NAME_SIZE		64

typedef struct _Stream
{
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
} Stream;

typedef struct
{
	unsigned int	portId;
	pcap_if_t		*dev;
	Stream			*streamHead;
	Stream			*streamTail;
} PortInfo;

class RxTx
{
	public:
		RxTx(AbstractHost* host);
		~RxTx();
		void ProcessMsg(const char* msg, int len);

	private:
		AbstractHost	*host;
		char			logStr[1024];

#define MAX_PKT_SIZE	1024
		unsigned char	pktBuff[MAX_PKT_SIZE];
		unsigned 		numPorts;
		PortInfo		*portInfo;
		pcap_if_t		*alldevs;

		void InsertStreamAtHead(unsigned int port, Stream *s);
		void InsertStreamAtTail(unsigned int port, Stream *s);
		bool InsertStreamBefore(unsigned int port, Stream *s, 
			unsigned int nextStreamId);
		bool DeleteStream(unsigned int port, Stream *s);
		bool DeleteStream(unsigned int port, unsigned int streamId);
		void DeleteAllStreams(unsigned int port);
		Stream* GetStream(unsigned int port, unsigned int streamId);

		//void Log(char *fmt, ...);
		void SendCapabilityInfo(void);
		void SendPortInfo(unsigned int port);
		void ProcessPortConfig(const char* msg, int len);
};

#endif
#endif
