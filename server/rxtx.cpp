
File Not used anymore

#if 0
#include "qtglobal" // FIXME: needed only for qdebug
#include "rxtx.h"
#if 0 // PB
#include "../common/protocol.h"
#endif



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
		portInfo[i].streamHead = NULL;
		portInfo[i].streamTail = NULL;
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
	unsigned int i;

	for (i = 0; i < numPorts; i++)
		DeleteAllStreams(i);
    pcap_freealldevs(alldevs);
}

void RxTx::ProcessMsg(const char* msg, int len)
{
	tCommHdr *hdr;
	// TODO: For now assuming we'll get a complete msg
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
		case e_MT_GetCapability:
			SendCapabilityInfo();
			break;
		case e_MT_ChangePortConfig:
			ProcessPortConfig(msg+sizeof(tCommHdr), len - sizeof(tCommHdr));
			break;
		case e_MT_GetPortConfig:
			SendPortInfo(0);	// FIXME
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
		((tTlvPortCapability*)(p))->portId = HTONL(portInfo[i].portId);
		((tTlvPortCapability*)(p))->portSpeed = 0; // TODO
#if 0
		strncpy(((tTlvPortCapability*)(p))->name, 
			portInfo[i].dev->name, TLV_MAX_PORT_NAME);
		((tTlvPortCapability*)(p))->name[TLV_MAX_PORT_NAME-1] = 0; 
#else
		strcpy(((tTlvPortCapability*)(p))->portName, "eth");
		//strcat(((tTlvPortCapability*)(p))->name, itoa(portInfo[i].portId, NULL, 10));
		itoa(portInfo[i].portId, &(((tTlvPortCapability*)(p))->portName[3]), 10);
#endif
		strncpy(((tTlvPortCapability*)(p))->portDesc,
			portInfo[i].dev->description, TLV_MAX_PORT_DESC);
		((tTlvPortCapability*)(p))->portDesc[TLV_MAX_PORT_DESC -1] = 0;
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
#if 0 // PB
	host->SendMsg(pktBuff, msgLen);
#endif
}

void RxTx::ProcessPortConfig(const char* msg, int len)
{
	// ASSUMPTION: msg points to start of first TLV
	UINT8		*p = (UINT8*) msg;
	uTlvStream	u;
	Stream		*s;

	// Extract and process each TLV
	while (len)
	{
		if (len < 12)
		{
			LOG("Length (%d) Error - not enough to fit a TLV", len);
			goto _exit;
		}

		u.tlv.tlvType = NTOHS(GET16(p));
		u.tlv.tlvLen = NTOHS(GET16(p+2));
		u.tlv.portId = NTOHL(GET32(p+4));
		u.tlv.streamId = NTOHL(GET32(p+8));

		p += 12;
		len -= 12;

		// Locate the correct node for processing
		if (u.tlv.portId >= numPorts)
			goto _next_tlv;

		s = GetStream(u.tlv.portId, u.tlv.streamId);
		if ((s == NULL) && (u.tlv.tlvType!= e_TT_StreamOper))
		{
			LOG("Unrecognized stream Id %d\n", u.tlv.streamId);
			goto _next_tlv;
		}

		switch(u.tlv.tlvType)
		{
			case e_TT_StreamOper:
				u.oper.streamOper = NTOHS(GET16(p+2));
				switch (u.oper.streamOper)
				{
					case TLV_STREAM_OPER_DELETE:
						if (!DeleteStream(u.tlv.portId, u.tlv.streamId))
						{
							LOG("No Stream with id %d currently in list\n", 
								u.tlv.streamId);
							goto _next_tlv;
						}
						break;
					case TLV_STREAM_OPER_INSERT_HEAD:
						s = new Stream;
						s->id = u.tlv.streamId;

						InsertStreamAtHead(u.tlv.portId, s);
						break;
					case TLV_STREAM_OPER_INSERT_TAIL:
						s = new Stream;
						s->id = u.tlv.streamId;

						InsertStreamAtTail(u.tlv.portId, s);
						break;
					case TLV_STREAM_OPER_INSERT_BEFORE:
					{
						UINT32	nextStreamId;

						s = new Stream;
						s->id = u.tlv.streamId;
							
						nextStreamId = NTOHS(GET32(p+4));

						if (!InsertStreamBefore(u.tlv.portId, s, nextStreamId))
						{
							LOG("List Empty or No stream with id %d "
								"currently in list\n", nextStreamId);
							goto _next_tlv;
						}
						break;
					}
					default:
						LOG("Unrecognized Stream Oper %d\n",
							u.oper.streamOper);
						goto _next_tlv;
				}
				break;
			case e_TT_StreamName:
				strncpy(s->name, (char*) p, MAX_STREAM_NAME_SIZE);
				break;

			case e_TT_StreamStatus:
				u.status.streamStatus = NTOHL(GET32(p));
				if (u.status.streamStatus == TLV_STREAM_STATUS_DISABLED)
					s->flags |= STREAM_FLAG_VALUE_STATUS_DISABLED; // FIXME
				else if (u.status.streamStatus == TLV_STREAM_STATUS_ENABLED)
					s->flags |= STREAM_FLAG_VALUE_STATUS_ENABLED; // FIXME
				else
					goto _next_tlv;
				break;

			case e_TT_StreamFrameLength:
				u.frameLen.frameLenMode = NTOHS(GET16(p));
				u.frameLen.frameLen = NTOHS(GET16(p+2));
				u.frameLen.frameLenMin = NTOHS(GET16(p+4));
				u.frameLen.frameLenMax = NTOHS(GET16(p+6));

				s->pktLen = u.frameLen.frameLen;

				// FIXME: other frameLen params
				break;

			case e_TT_StreamDataPattern:
				u.dataPattern.dataPatternMode = NTOHS(GET16(p));
				u.dataPattern.dataPattern = NTOHS(GET32(p+4));
				
				s->dataPattern = u.dataPattern.dataPattern;

				// FIXME: other dataPattern params
				break;

			case e_TT_StreamHeaderData:
				u.headerData.headerLen = NTOHS(GET16(p+2));

				s->hdrLen = u.headerData.headerLen;
				memcpy(s->pktHdr, p+4, u.headerData.headerLen);
				break;

			default:
				LOG("Unrecognizeed/Unexpected TLV %d\n", u.tlv.tlvType);
		}

_next_tlv:
		p += u.tlv.tlvLen;
		len -= u.tlv.tlvLen;
	}

_exit:
	return;
}

void RxTx::SendPortInfo(unsigned int port)
{
	// FIXME
}

/*
** --------------------- STREAM LIST OPERATIONS -------------------------
*/

void RxTx::InsertStreamAtHead(unsigned int port, Stream *s)
{
	if (portInfo[port].streamHead == NULL)
	{
		// list empty - first entry being added
		s->next = NULL;
		portInfo[port].streamHead = portInfo[port].streamTail = s;
	}
	else
	{
		// at least one entry in list, so tail does not change
		s->next = portInfo[port].streamHead;
		portInfo[port].streamHead = s;
	}
}

void RxTx::InsertStreamAtTail(unsigned int port, Stream *s)
{
	s->next = NULL;
	if (portInfo[port].streamHead == NULL)
	{
		// list empty - first entry being added
		portInfo[port].streamHead = portInfo[port].streamTail = s;
	}
	else
	{
		// at least one entry in list, so head does not change
		portInfo[port].streamTail->next = s;
		portInfo[port].streamTail = s;
	}
}

bool RxTx::InsertStreamBefore(unsigned int port, Stream *s, 
	unsigned int nextStreamId)
{
	Stream	*q, *r;

	// For an "Insert Before", list cannot be empty
	if (portInfo[port].streamHead == NULL)
	{
		LOG("Cannot 'insert before' in an empty list");
		return false;
	}

	// Traverse with 'r' and keep track of previous with 'q'
	q = NULL;
	r = portInfo[port].streamHead;
	while (r != NULL)
	{
		if (r->id == nextStreamId)
		{
			if (r == portInfo[port].streamHead)
			{
				// Insert at Head
				s->next = portInfo[port].streamHead;
				portInfo[port].streamHead = s;
			}
			else if (r == portInfo[port].streamTail)
			{
				// Insert one before Tail
				s->next = portInfo[port].streamTail;
				q->next = s;	
			}
			else
			{
				s->next = r;
				q->next = s;
			}

			break;
		}
		q = r;
		r = r->next;
	}

	if (r == NULL)
		return false;
	else
		return true;
}

bool RxTx::DeleteStream(unsigned int port, Stream *s)
{
	Stream *q, *r;

	// Traverse with 'r' and keep track of prev with 'q'
	q = NULL;
	r = portInfo[port].streamHead;
	while (r != NULL)
	{
		if (r == s)
		{
			if (r == portInfo[port].streamHead)
			{
				if (portInfo[port].streamHead == portInfo[port].streamTail)
				{
					portInfo[port].streamHead = NULL;
					portInfo[port].streamTail = NULL;
				}
				else
					portInfo[port].streamHead = portInfo[port].streamHead->next;
			}
			else if (r == portInfo[port].streamTail)
			{
				q->next = NULL;
				portInfo[port].streamTail = q;
			}
			else
			{
				q->next = r->next;
			}

			delete r;
			break;
		}
		q = r;
		r = r->next;
	}
	
	if (r == NULL)
		return false;
	else
		return true;
}

bool RxTx::DeleteStream(unsigned int port, unsigned int streamId)
{
	Stream *q, *r;

	// Traverse with 'r' and keep track of prev with 'q'
	q = NULL;
	r = portInfo[port].streamHead;
	while (r != NULL)
	{
		if (r->id == streamId)
		{
			if (r == portInfo[port].streamHead)
			{
				if (portInfo[port].streamHead == portInfo[port].streamTail)
				{
					portInfo[port].streamHead = NULL;
					portInfo[port].streamTail = NULL;
				}
				else
					portInfo[port].streamHead = portInfo[port].streamHead->next;
			}
			else if (r == portInfo[port].streamTail)
			{
				q->next = NULL;
				portInfo[port].streamTail = q;
			}
			else
			{
				q->next = r->next;
			}

			delete r;
			break;
		}
		q = r;
		r = r->next;
	}
	
	if (r == NULL)
		return false;
	else
		return true;
}

void RxTx::DeleteAllStreams(unsigned int port)
{
	Stream *r, *q;

	r = portInfo[port].streamHead;
	while (r != NULL)
	{
		q = r;
		r = r->next;
		delete q;
	}
}

Stream* RxTx::GetStream(unsigned int port, unsigned int streamId)
{
	Stream *r;

	r = portInfo[port].streamHead;
	while (r != NULL)
	{
		if (r->id == streamId)
			return r;
		r = r->next;
	}
	
	return NULL;
}
#endif
