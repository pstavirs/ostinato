#include "rxtx.h"
#include "../common/protocol.h"
#include "qtglobal" // FIXME: needed only for qdebug

//#define LOG(...)   drone->ui.teLOG->append(QString().sprintf( __VA_ARGS__))
//#define LOG(...)   drone->LOG(QString().sprintf( __VA_ARGS__))
#define LOG(...)   {sprintf(logStr, __VA_ARGS__); host->Log(logStr);}


RxTx::RxTx(AbstractHost *host)
{
    pcap_if_t *d;
    int i=0;
    char errbuf[PCAP_ERRBUF_SIZE];

	// Init Data
	RxTx::host = host;
	numPorts = 0;
	alldevs = NULL;

    LOG("Retrieving the device list from the local machine\n"); 
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
    {
        LOG("Error in pcap_findalldevs_ex: %s\n", errbuf);
        goto _fail;
    }

	/* Count number of local ports */
    for(d = alldevs; d != NULL; d = d->next)
		numPorts++;
   
   	portInfo = new PortInfo[numPorts];

    /* Print the list */
    for(i=0, d=alldevs; d!=NULL; i++, d=d->next)
    {
		portInfo[i].portId = i;
		portInfo[i].dev = d;
#if 1
        LOG("%d. %s", i, d->name);
        if (d->description)
		{
            LOG(" (%s)\n", d->description);
		}
        else
            LOG(" (No description available)\n");
#endif
    }
    
    if (i == 0)
    {
        LOG("\nNo interfaces found! Make sure WinPcap is installed.\n");
        goto _fail;
    }

_fail:
	return;
}

#if 0
RxTx::LOG(char* fmt, ...)
{
	sprintf(logStr, fmt, _VA_ARGS_);
	host->LOG(logStr);
}
#endif

RxTx::~RxTx()
{
    pcap_freealldevs(alldevs);
}

void RxTx::ProcessMsg(const char* msg, int len)
{
	tCommHdr *hdr;
	// TODO: For now, assuming we'll get a complete msg
	// but need to fix this as this is a TCP stream

	hdr = (tCommHdr*) msg;

	if (hdr->ver != 1) // FIXME:hardcoding
	{
		LOG("Rcvd msg with invalid version %d\n", hdr->ver);
		goto _exit;
	}

	qDebug("msgType - %x: %x\n", hdr->msgType, NTOHS(hdr->msgType));
	switch (NTOHS(hdr->msgType))
	{
		case e_MT_CapabilityReq:
			SendCapabilityInfo();
			break;

		default:
			LOG("Rcvd msg with unrecognized msgType %d\n", NTOHS(hdr->msgType));
	}
	
_exit:
	return;
}

void RxTx::SendCapabilityInfo(void)
{
	unsigned char *msg, *p;
	unsigned int i, msgLen;

	p = msg = (unsigned char*) pktBuff;
	((tCommHdr*)(p))->ver = 1;
	((tCommHdr*)(p))->msgType = HTONS(e_MT_CapabilityInfo);
	p += sizeof(tCommHdr);

	for (i = 0; i < numPorts; i++)
	{
		// TLV: Port Capability
		((tTlvPortCapability*)(p))->tlvType = HTONS(e_TT_PortCapability);
		((tTlvPortCapability*)(p))->tlvLen = HTONS(sizeof(tTlvPortCapability));
		((tTlvPortCapability*)(p))->port = HTONL(portInfo[i].portId);
		((tTlvPortCapability*)(p))->speed = 0; // TODO
#if 0
		strncpy(((tTlvPortCapability*)(p))->name, 
			portInfo[i].dev->name, TLV_MAX_PORT_NAME);
		((tTlvPortCapability*)(p))->name[TLV_MAX_PORT_NAME-1] = 0; 
#else
		strcpy(((tTlvPortCapability*)(p))->name, "eth");
		//strcat(((tTlvPortCapability*)(p))->name, itoa(portInfo[i].portId, NULL, 10));
		itoa(portInfo[i].portId, &(((tTlvPortCapability*)(p))->name[3]), 10);
#endif
		strncpy(((tTlvPortCapability*)(p))->desc,
			portInfo[i].dev->description, TLV_MAX_PORT_DESC);
		((tTlvPortCapability*)(p))->desc[TLV_MAX_PORT_DESC -1] = 0;
		p += sizeof(tTlvPortCapability);
	}
	msgLen = (p - msg);
	((tCommHdr*)(msg))->msgLen = HTONS(msgLen);

	logStr[0] = 0;
	for (i = 0; i < msgLen >> 2; i++)
	{
		char word[10];

		sprintf(word, "%08X ", HTONL(((unsigned int *)(msg))[i]));
		strcat(logStr, word);
	}
	host->Log("Sending msg\n");
	host->Log(logStr);
	host->SendMsg(pktBuff, msgLen);
}
