#ifndef _RXTX_H
#define _RXTX_H

#include "pcap.h"
#include "abstracthost.h"


typedef struct
{
	unsigned int	portId;
	pcap_if_t		*dev;
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

		//void Log(char *fmt, ...);
		void SendCapabilityInfo(void);
};

#endif
