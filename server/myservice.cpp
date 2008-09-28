#include "myservice.h"
#include "qdebug.h"

#include <qglobal.h>
#include <qendian.h>

#ifdef Q_OS_WIN32
#include <pcap-int.h>
#include <Ntddndis.h>
#endif

#define LOG(...)	{sprintf(logStr, __VA_ARGS__); host->Log(logStr);}
#define MB			(1024*1024)

#if 0
quint16 StreamInfo::ipv4Cksum(quint16 ipHdrLen, quint16 buff[])
{
	quint16 word16;
	quint32 sum=0;
	quint16 i;
    
	// make 16 bit words out of every two adjacent 8 bit words in the packet
	// and add them up
	for (i = 0; i < ipHdrLen ;i += 2)
	{	
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (quint32) word16;	
	}
	
	// take only 16 bits out of the 32 bit sum and add up the carries
	while (sum>>16)
	  sum = (sum & 0xFFFF)+(sum >> 16);

	// one's complement the result
	sum = ~sum;
	
	return (quint16) sum;
}
#endif

quint16 StreamInfo::ipv4Cksum(uchar *buf, int len)
{
	quint32 sum = 0;  /* assume 32 bit long, 16 bit short */
	quint16 *ip = (quint16*) buf;

	while(len > 1)
	{
		sum += *ip;
		if(sum & 0x80000000)   /* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		ip++;
		len -= 2;
	}

	if(len)       /* take care of left over byte */
		sum += (unsigned short) *(unsigned char *)ip;

	while(sum>>16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	qDebug("cksum = %x", ~sum);
	return (quint16) ~sum;
}

int StreamInfo::makePacket(uchar *buf, int bufMaxSize, int n)
{
	int		u, pktLen, len = 0;
	uchar	scratch[8];

	// TODO(HI): use FrameLengthMode - don't assume fixed
	// pktLen is adjusted for CRC/FCS which will be added by the NIC
	pktLen = d.core().frame_len() - 4;
	if (bufMaxSize < pktLen)
		return 0;

	// We always have a Mac Header!
	switch (d.mac().dst_mac_mode())
	{
		case OstProto::Mac::e_mm_fixed:
			qToBigEndian((quint64) d.mac().dst_mac(), scratch);
			break;
		case OstProto::Mac::e_mm_inc:
			u = (n % d.mac().dst_mac_count()) * d.mac().dst_mac_step(); 
			qToBigEndian((quint64) d.mac().dst_mac() + u, scratch);
			break;
		case OstProto::Mac::e_mm_dec:
			u = (n % d.mac().dst_mac_count()) * d.mac().dst_mac_step(); 
			qToBigEndian((quint64) d.mac().dst_mac() - u, scratch);
			break;
		default:
			qWarning("Unhandled dstMac_mode %d", d.mac().dst_mac_mode());
	}
	memcpy((buf + len), scratch + 2, 6);
	len += 6;

	switch (d.mac().src_mac_mode())
	{
		case OstProto::Mac::e_mm_fixed:
			qToBigEndian((quint64) d.mac().src_mac(), scratch);
			break;
		case OstProto::Mac::e_mm_inc:
			u = (n % d.mac().src_mac_count()) * d.mac().src_mac_step(); 
			qToBigEndian((quint64) d.mac().src_mac() + u, scratch);
			break;
		case OstProto::Mac::e_mm_dec:
			u = (n % d.mac().src_mac_count()) * d.mac().src_mac_step(); 
			qToBigEndian((quint64) d.mac().src_mac() - u, scratch);
			break;
		default:
			qWarning("Unhandled srcMac_mode %d", d.mac().src_mac_mode());
	}
	memcpy((buf + len), scratch + 2, 6);
	len += 6;


	// Frame Type - Part 1 (pre VLAN info)
	switch(d.core().ft())
	{
	case OstProto::StreamCore::e_ft_none:
	case OstProto::StreamCore::e_ft_eth_2:
		break;
	case OstProto::StreamCore::e_ft_802_3_raw:
		qToBigEndian((quint16) pktLen, buf+len);
		len += 2;
		break;
	case OstProto::StreamCore::e_ft_802_3_llc:
		qToBigEndian((quint16) pktLen, buf+len);
		len += 2;
		buf[len+0] = (quint8) d.llc().dsap();
		buf[len+1] = (quint8) d.llc().ssap();
		buf[len+2] = (quint8) d.llc().ctl();
		len +=3;
		break;
	case OstProto::StreamCore::e_ft_snap:
		qToBigEndian((quint16) pktLen, buf+len);
		len += 2;
		buf[len+0] = (quint8) d.llc().dsap();
		buf[len+1] = (quint8) d.llc().ssap();
		buf[len+2] = (quint8) d.llc().ctl();
		len +=3;
		qToBigEndian((quint32) d.snap().oui(), scratch);
		memcpy((buf + len), scratch + 2, 3);
		len += 3;
		break;
	default:
		qWarning("Unhandled frame type %d\n", d.core().ft());
	}

	// VLAN
	if (d.vlan().is_svlan_tagged())
	{
		if (d.vlan().is_stpid_override())
			qToBigEndian((quint16) d.vlan().stpid(), buf+len);
		else
			qToBigEndian((quint16) 0x88a8, buf+len);
		len += 2 ;

		qToBigEndian((quint16) d.vlan().svlan_tag(), buf+len);
		len += 2 ;
	}

	if (d.vlan().is_cvlan_tagged())
	{
		if (d.vlan().is_ctpid_override())
			qToBigEndian((quint16) d.vlan().ctpid(), buf+len);
		else
			qToBigEndian((quint16) 0x8100, buf+len);
		len += 2 ;

		qToBigEndian((quint16) d.vlan().cvlan_tag(), buf+len);
		len += 2 ;
	}

	// Frame Type - Part 2 (post VLAN info)
	switch(d.core().ft())
	{
	case OstProto::StreamCore::e_ft_none:
		break;
	case OstProto::StreamCore::e_ft_eth_2:
		qToBigEndian((quint16) d.eth2().type(), buf+len);
		len += 2;
		break;
	case OstProto::StreamCore::e_ft_802_3_raw:
	case OstProto::StreamCore::e_ft_802_3_llc:
		break;
	case OstProto::StreamCore::e_ft_snap:
		qToBigEndian((quint16) d.eth2().type(), buf+len);
		len += 2;
		break;
	default:
		qWarning("Unhandled frame type %d\n", d.core().ft());
	}

	// L3
	switch (d.core().l3_proto())
	{
	case OstProto::StreamCore::e_l3_none:
		break;
	case OstProto::StreamCore::e_l3_ip:
	{
		quint32 subnet, host;
		int ipOfs = len;

		buf[len+0] = (quint8) (d.ip().ver_hdrlen());
		buf[len+1] = (quint8) (d.ip().tos());
		len += 2;

		if (d.ip().is_override_totlen())
			qToBigEndian((quint16) d.ip().tot_len(), buf+len);
		else
			qToBigEndian((quint16) (pktLen - ipOfs), buf+len);
		len += 2;

		qToBigEndian((quint16) d.ip().id(), buf+len);
		len += 2;

		qToBigEndian((quint16) (( (d.ip().flags() & 0x3) << 13) | 
					(d.ip().frag_ofs() & 0x1FFF)), buf+len);
		len += 2;

		buf[len+0] = (quint8) (d.ip().ttl());
		buf[len+1] = (quint8) (d.ip().proto());
		len += 2;

		// cksum calculated after filling in the rest
		qToBigEndian((quint16) 0, buf+len);
		len += 2;

		// TODO(HI): Use IpMode - don't assume fixed
		switch(d.ip().src_ip_mode())
		{
			case OstProto::Ip::e_im_fixed:
				qToBigEndian((quint32) d.ip().src_ip(), buf+len);
				break;
			case OstProto::Ip::e_im_inc_host:
				u = n % d.ip().src_ip_count();
				subnet = d.ip().src_ip() & d.ip().src_ip_mask();
				host = (((d.ip().src_ip() & ~d.ip().src_ip_mask()) + u) &
					~d.ip().src_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			case OstProto::Ip::e_im_dec_host:
				u = n % d.ip().src_ip_count();
				subnet = d.ip().src_ip() & d.ip().src_ip_mask();
				host = (((d.ip().src_ip() & ~d.ip().src_ip_mask()) - u) &
					~d.ip().src_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			case OstProto::Ip::e_im_random_host:
				subnet = d.ip().src_ip() & d.ip().src_ip_mask();
				host = (qrand() & ~d.ip().src_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			default:
				qWarning("Unhandled src_ip_mode = %d", d.ip().src_ip_mode());
		}
		len +=4;

		switch(d.ip().dst_ip_mode())
		{
			case OstProto::Ip::e_im_fixed:
				qToBigEndian((quint32) d.ip().dst_ip(), buf+len);
				break;
			case OstProto::Ip::e_im_inc_host:
				u = n % d.ip().dst_ip_count();
				subnet = d.ip().dst_ip() & d.ip().dst_ip_mask();
				host = (((d.ip().dst_ip() & ~d.ip().dst_ip_mask()) + u) &
					~d.ip().dst_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			case OstProto::Ip::e_im_dec_host:
				u = n % d.ip().dst_ip_count();
				subnet = d.ip().dst_ip() & d.ip().dst_ip_mask();
				host = (((d.ip().dst_ip() & ~d.ip().dst_ip_mask()) - u) &
					~d.ip().dst_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			case OstProto::Ip::e_im_random_host:
				subnet = d.ip().dst_ip() & d.ip().dst_ip_mask();
				host = (qrand() & ~d.ip().dst_ip_mask());
				qToBigEndian((quint32) (subnet | host), buf+len);
				break;
			default:
				qWarning("Unhandled dst_ip_mode = %d", d.ip().dst_ip_mode());
		}
		len +=4;

		// Calculate and fill in cksum (unless overridden)
		if (d.ip().is_override_cksum())
			qToBigEndian((quint16) d.ip().cksum(), buf+ipOfs+10);
		else
			*((quint16*)(buf + ipOfs + 10)) = ipv4Cksum(buf + ipOfs, len-ipOfs);
		break;
	}
	case OstProto::StreamCore::e_l3_arp:
		// TODO(LOW)
		break;
	default:
		qWarning("Unhandled l3 proto %d\n", d.core().l3_proto());
	}

	switch (d.core().l4_proto())
	{
	case OstProto::StreamCore::e_l4_none:
		break;
	case OstProto::StreamCore::e_l4_tcp:
	{
		qToBigEndian((quint16) d.tcp().src_port(), buf+len);
		len += 2;
		qToBigEndian((quint16) d.tcp().dst_port(), buf+len);
		len += 2;

		qToBigEndian((quint32) d.tcp().seq_num(), buf+len);
		len += 4;
		qToBigEndian((quint32) d.tcp().ack_num(), buf+len);
		len += 4;

		if (d.tcp().is_override_hdrlen())
			buf[len+0] = (quint8) d.tcp().hdrlen_rsvd();
		else
			buf[len+0] = (quint8) 0x50; // FIXME(LOW)
		buf[len+1] = (quint8) d.tcp().flags();
		len += 2;

		qToBigEndian((quint16) d.tcp().window(), buf+len);
		len +=2;

		if (d.tcp().is_override_cksum())
			qToBigEndian((quint16) d.tcp().cksum(), buf+len);
		else
			qToBigEndian((quint16) 0, buf+len); // FIXME(HI)
		len +=2;

		qToBigEndian((quint16) d.tcp().urg_ptr(), buf+len);
		len +=2;
		break;
	}
	case OstProto::StreamCore::e_l4_udp:
	{
		int udpLen = pktLen - len;

		qToBigEndian((quint16) d.udp().src_port(), buf+len);
		len += 2;
		qToBigEndian((quint16) d.udp().dst_port(), buf+len);
		len += 2;

		if (d.udp().is_override_totlen())
			qToBigEndian((quint16) d.udp().totlen(), buf+len);
		else
			qToBigEndian((quint16) udpLen, buf+len);
		len +=2;

		if (d.udp().is_override_cksum())
			qToBigEndian((quint16) d.udp().cksum(), buf+len);
		else
			qToBigEndian((quint16) 0, buf+len); // FIXME(HI)
		len +=2;
		break;
	}
	case OstProto::StreamCore::e_l4_icmp:
		// TODO(LOW)
		break;
	case OstProto::StreamCore::e_l4_igmp:
		// TODO(LOW)
		break;
	default:
		qWarning("Unhandled l4 proto %d\n", d.core().l4_proto());
	}

	// Fill-in the data pattern
	{
		int dataLen;

		dataLen = pktLen - len;
		for (int i = 0; i < (dataLen/4)+1; i++)
		{
			// TODO(HI): Use patternMode
			qToBigEndian((quint32) d.core().pattern(), buf+len+(i*4));
		}
	}

	return pktLen;
}


//
// ------------------ PortInfo --------------------
//
PortInfo::PortInfo(uint id, pcap_if_t *dev)
	: monitor(this)
{
    char errbuf[PCAP_ERRBUF_SIZE];

	this->dev = dev;
	devHandle = pcap_open(dev->name, 65536, PCAP_OPENFLAG_PROMISCUOUS , 1000, 
			NULL, errbuf);
	if (devHandle == NULL)
	{
		qDebug("Error opening port %s: %s\n", 
				dev->name, pcap_geterr(devHandle));
	}

#if 0
	if (pcap_setdirection(devHandle, PCAP_D_IN)<0)
	{
		qDebug("[%s] Error setting direction inbound only\n", dev->name);
	}
#endif

	/* By default, put the interface in statistics mode */
	if (pcap_setmode(devHandle, MODE_STAT)<0)
	{
		qDebug("Error setting statistics mode.\n");
	}

	d.mutable_port_id()->set_id(id);
	d.set_name("eth"); // FIXME(MED): suffix portid
	d.set_description(dev->description);
	d.set_is_enabled(true);	// FIXME(MED):check
	d.set_is_oper_up(true); // FIXME(MED):check
	d.set_is_exclusive_control(false); // FIXME(MED): check

	memset((void*) &stats, 0, sizeof(stats));
	resetStats();

	// We'll create sendqueue later when required
	sendQueue = NULL;
	pcapExtra.sendQueueNumPkts = 0;
	pcapExtra.txPkts = 0;
	pcapExtra.txBytes = 0;
	isSendQueueDirty=true;

	// Start the monitor thread
	monitor.start();
}

void PortInfo::update()
{
	uchar		pktBuf[2000];
	pcap_pkthdr	pktHdr;

	qDebug("In %s", __FUNCTION__);

	if (sendQueue)
		pcap_sendqueue_destroy(sendQueue);

	// TODO(LOW): calculate sendqueue size
	sendQueue = pcap_sendqueue_alloc(1*MB);
	pcapExtra.sendQueueNumPkts = 0;
	
	// First sort the streams by ordinalValue
	qSort(streamList);

	for (int i = 0; i < streamList.size(); i++)
	{
		if (streamList[i].d.core().is_enabled())
		{
			int	numPackets, numBursts;

			if (streamList[i].d.control().unit() == 
					OstProto::StreamControl::e_su_bursts)
			{
				numBursts = streamList[i].d.control().num_bursts();
				numPackets = streamList[i].d.control().packets_per_burst();
			}
			else
			{
				numBursts = 1;
				numPackets = streamList[i].d.control().num_packets();
			}

			for (int j = 0; j < numBursts; j++)
			{
				for (int k = 0; k < numPackets; k++)
				{
					pktHdr.len = streamList[i].makePacket(
							pktBuf, sizeof(pktBuf), j * numPackets + k);
					pktHdr.caplen = pktHdr.len;
					pktHdr.ts.tv_sec = pktHdr.ts.tv_usec = 0; // FIXME(HI)

					if (-1 == pcap_sendqueue_queue(sendQueue, &pktHdr, 
								(u_char*) pktBuf))
					{
						qDebug("[port %d] sendqueue_queue() failed for "
								"streamidx %d\n", id(), i);
					}
					else
						pcapExtra.sendQueueNumPkts++;
				}
			}
		}
	}

	isSendQueueDirty = false;
}

void PortInfo::startTransmit()
{
	uint bytes;

	// TODO(HI): Stream Mode - one pass/continuous
	bytes = pcap_sendqueue_transmit(devHandle, sendQueue, false);
	if (bytes < sendQueue->len)
	{	
		qDebug("port %d: sent (%d/%d) error %s. TxStats may be inconsistent", 
				id(), bytes, sendQueue->len, pcap_geterr(devHandle));
		//! \TODO parse sendqueue using 'bytes' to get actual pkts sent
		pcapExtra.txPkts += pcapExtra.sendQueueNumPkts;
		pcapExtra.txBytes += bytes;
	}
	else
	{
		qDebug("port %d: sent (%d/%d) bytes\n", id(), bytes, sendQueue->len);
		pcapExtra.txPkts += pcapExtra.sendQueueNumPkts;
		pcapExtra.txBytes += bytes;
	}

	// pcap_sendqueue_transmit() returned 'bytes' includes size of pcap_pkthdr
	// - adjust for it
	pcapExtra.txBytes -= pcapExtra.sendQueueNumPkts * sizeof(pcap_pkthdr);

}

void PortInfo::stopTransmit()
{
}

void PortInfo::resetStats()
{
	memcpy((void*) &epochStats, (void*) &stats, sizeof(stats));
}

//
// ------------------ PortMonitor -------------------
//

PortInfo::PortMonitor::PortMonitor(PortInfo *port)
{
	this->port = port;
#ifdef Q_OS_WIN32
	{
		int sz = sizeof(PACKET_OID_DATA) + sizeof(quint64) + 4;
		//oidData = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
			//sizeof(PACKET_OID_DATA) + sizeof(quint64) - 1);
		oidData = (PPACKET_OID_DATA) malloc(sz);
		if (oidData)
		{
			memset(oidData, 0, sz);
			oidData->Length=sizeof(quint64);
		}
		else
			qFatal("failed to alloc oidData");
	}
#endif
}

void PortInfo::PortMonitor::callback(u_char *state, 
		const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	uint		usec;
	PortInfo	*port = (PortInfo*) state;

	quint64 pkts;
	quint64 bytes;

	// Update RxStats and RxRates using PCAP data
	pkts  = *((quint64*)(pkt_data + 0));
	bytes = *((quint64*)(pkt_data + 8));

	// Note: PCAP reported bytes includes ETH_FRAME_HDR_SIZE - adjust for it
	bytes -= pkts * ETH_FRAME_HDR_SIZE;

	usec = (header->ts.tv_sec - port->lastTs.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTs.tv_usec);
	port->stats.rxPps = (pkts * 1000000) / usec;
	port->stats.rxBps = (bytes * 1000000) / usec;

	port->stats.rxPkts += pkts;
	port->stats.rxBytes += bytes;

	// Update TxStats from PcapExtra
	port->stats.txPkts = port->pcapExtra.txPkts;
	port->stats.txBytes = port->pcapExtra.txBytes;

	//! \TODO TxRates

	// Store curr timestamp as last timestamp
	port->lastTs.tv_sec = header->ts.tv_sec;
	port->lastTs.tv_usec = header->ts.tv_usec;

#if 0
	for (int i=0; i < 16; i++)
	{
		qDebug("%02x ", pkt_data[i]);
	}
	qDebug("{%d: %llu, %llu}\n", port->id(),
			pkts, bytes);
	qDebug("[%d: pkts : %llu]\n", port->id(), port->stats.rxPkts);
	qDebug("[%d: bytes: %llu]\n", port->id(), port->stats.rxBytes);
#endif

	// Retreive NIC stats
#ifdef Q_OS_WIN32
	port->monitor.oidData->Oid = OID_GEN_RCV_OK;
	if (PacketRequest(port->devHandle->adapter, 0, port->monitor.oidData))
	{
		if (port->monitor.oidData->Length <= sizeof(port->stats.txPkts))
			memcpy((void*)&port->stats.rxPktsNic,
					(void*)port->monitor.oidData->Data, 
					port->monitor.oidData->Length);
	}
	port->monitor.oidData->Oid = OID_GEN_XMIT_OK;
	if (PacketRequest(port->devHandle->adapter, 0, port->monitor.oidData))
	{
		if (port->monitor.oidData->Length <= sizeof(port->stats.txPkts))
			memcpy((void*)&port->stats.txPktsNic,
					(void*)port->monitor.oidData->Data, 
					port->monitor.oidData->Length);
	}
#endif
}

void PortInfo::PortMonitor::run()
{
	int		ret;

	qDebug("before pcap_loop\n");

	/* Start the main loop */
	ret = pcap_loop(port->devHandle, -1, &PortInfo::PortMonitor::callback, 
			(PUCHAR) port);
	//ret = pcap_loop(fp, -1, &updateStats, (PUCHAR)&st_ts);

	switch(ret)
	{
		case 0:
			qDebug("Unexpected return from pcap_loop()\n");
			break;
		case -1:
			qDebug("Unsolicited (error) return from pcap_loop()\n");
			break;
		case -2:
			qDebug("Solicited return from pcap_loop()\n");
			break;
		default:
			qDebug("Unknown return value from pcap_loop()\n");
	}
}

/*--------------- MyService ---------------*/

int MyService::getStreamIndex(unsigned int portIdx,
	unsigned int streamId)
{
	int i;

	// note: index and id are interchageable for port but not for stream

	Q_ASSERT(portIdx < numPorts);

	for (i = 0; i < portInfo[portIdx]->streamList.size(); i++)
	{
		if (streamId == portInfo[portIdx]->streamList.at(i).d.stream_id().id())
			goto _found;
	}

	qDebug("%s: stream id %d not found", __PRETTY_FUNCTION__, streamId);
	return -1;

_found:
	return i;
}

MyService::MyService(AbstractHost *host)
{
    pcap_if_t *dev;
    int i=0;
    char errbuf[PCAP_ERRBUF_SIZE];

	// Init Data
	this->host = host;
	numPorts = 0;
	alldevs = NULL;

    LOG("Retrieving the device list from the local machine\n"); 
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
    {
        LOG("Error in pcap_findalldevs_ex: %s\n", errbuf);
        goto _fail;
    }

	portInfo.clear();
    /* Count, Populate and Print the list */
    for(i=0, dev=alldevs; dev!=NULL; i++, dev=dev->next)
    {
		portInfo.append(new PortInfo(i, dev));
		numPorts++;

#if 1
        LOG("%d. %s", i, dev->name);
        if (dev->description)
		{
            LOG(" (%s)\n", dev->description);
		}
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

MyService::~MyService()
{
    pcap_freealldevs(alldevs);
}

void MyService::getPortIdList(
	::google::protobuf::RpcController* controller,
	const ::OstProto::Void* request,
	::OstProto::PortIdList* response,
	::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);

	for (uint i = 0; i < numPorts; i++)
	{
		::OstProto::PortId	*p;

		p = response->add_port_id();
		p->set_id(portInfo[i]->d.port_id().id());
	}

	done->Run();
}

void MyService::getPortConfig(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::PortConfigList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);

	for (int i=0; i < request->port_id_size(); i++)
	{
		unsigned int idx;

		idx = request->port_id(i).id();
		if (idx < numPorts)
		{
			OstProto::Port	*p;

			p = response->add_port();
			p->CopyFrom(portInfo[idx]->d);
		}
	}

	done->Run();
}

void MyService::getStreamIdList(::google::protobuf::RpcController* controller,
const ::OstProto::PortId* request,
::OstProto::StreamIdList* response,
::google::protobuf::Closure* done)
{
	unsigned int portIdx;

	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->id();
	if (portIdx >= numPorts)
	{
		qDebug("%s: Invalid port id %d", __PRETTY_FUNCTION__, portIdx);
		controller->SetFailed("Invalid Port Id");
		goto _exit;		// TODO(LOW): Partial status of RPC
	}

	response->mutable_port_id()->set_id(portIdx);
	for (int j = 0; j < portInfo[portIdx]->streamList.size(); j++)
	{
		OstProto::StreamId	*s, *q;

		q = portInfo[portIdx]->streamList[j].d.mutable_stream_id();

		s = response->add_stream_id();
		s->CopyFrom(*q);
	}

_exit:
	done->Run();
}

void MyService::getStreamConfig(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::StreamConfigList* response,
::google::protobuf::Closure* done)
{
	unsigned int portIdx;

	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->port_id().id();
	if (portIdx >= numPorts)
	{
		controller->SetFailed("invalid portid");
		goto _exit;
	}

	response->mutable_port_id()->set_id(portIdx);
	for (int i = 0; i < request->stream_id_size(); i++)
	{
		int streamIndex;
		OstProto::Stream	*s;

		streamIndex = getStreamIndex(portIdx, request->stream_id(i).id());
		if (streamIndex < 0)
			continue;		// TODO(LOW): Partial status of RPC

		s = response->add_stream();
		s->CopyFrom(portInfo[portIdx]->streamList[streamIndex].d);
	}

_exit:
	done->Run();
}

void MyService::addStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	unsigned int portIdx;

	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->port_id().id();
	if (portIdx >= numPorts)
	{
		controller->SetFailed("invalid portid");
		goto _exit;
	}

	for (int i = 0; i < request->stream_id_size(); i++)
	{
		int	streamIndex;
		StreamInfo		s;

		// If stream with same id as in request exists already ==> error!!
		streamIndex = getStreamIndex(portIdx, request->stream_id(i).id());
		if (streamIndex >= 0)
			continue;		// TODO(LOW): Partial status of RPC

		// Append a new "default" stream - actual contents of the new stream is
		// expected in a subsequent "modifyStream" request - set the stream id
		// now itself however!!!
		s.d.mutable_stream_id()->CopyFrom(request->stream_id(i));
		portInfo[portIdx]->streamList.append(s);

		// TODO(LOW): fill-in response "Ack"????
	}
	portInfo[portIdx]->setDirty(true);
_exit:
	done->Run();
}

void MyService::deleteStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	unsigned int portIdx;

	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->port_id().id();
	if (portIdx >= numPorts)
	{
		controller->SetFailed("invalid portid");
		goto _exit;
	}

	for (int i = 0; i < request->stream_id_size(); i++)
	{
		int	streamIndex;
		StreamInfo		s;

		streamIndex = getStreamIndex(portIdx, request->stream_id(i).id());
		if (streamIndex < 0)
			continue;		// TODO(LOW): Partial status of RPC

		portInfo[portIdx]->streamList.removeAt(streamIndex);

		// TODO(LOW): fill-in response "Ack"????
	}
	portInfo[portIdx]->setDirty(true);
_exit:
	done->Run();
}

void MyService::modifyStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamConfigList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	unsigned int	portIdx;

	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->port_id().id();
	if (portIdx >= numPorts)
	{
		controller->SetFailed("invalid portid");
		goto _exit;
	}

	for (int i = 0; i < request->stream_size(); i++)
	{
		int streamIndex;

		streamIndex = getStreamIndex(portIdx, 
			request->stream(i).stream_id().id());
		if (streamIndex < 0)
			continue;		// TODO(LOW): Partial status of RPC

		portInfo[portIdx]->streamList[streamIndex].d.MergeFrom(
			request->stream(i));

		// TODO(LOW): fill-in response "Ack"????
	}
	portInfo[portIdx]->setDirty(true);
_exit:
	done->Run();
}

void MyService::startTx(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);

	// If any of the ports in the request are dirty, first update them
	for (int i=0; i < request->port_id_size(); i++)
	{
		uint portIdx;

		portIdx = request->port_id(i).id();
		if (portIdx >= numPorts)
			continue; 	// FIXME(MED): partial RPC?

		if (portInfo[portIdx]->isDirty())
			portInfo[portIdx]->update();
	}

	for (int i=0; i < request->port_id_size(); i++)
	{
		uint portIdx;

		portIdx = request->port_id(i).id();
		if (portIdx >= numPorts)
			continue; 	// FIXME(MED): partial RPC?

		portInfo[portIdx]->startTransmit();
	}

	// TODO(LOW): fill-in response "Ack"????

	done->Run();
}

void MyService::stopTx(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);

	for (int i=0; i < request->port_id_size(); i++)
	{
		uint portIdx;

		portIdx = request->port_id(i).id();
		if (portIdx >= numPorts)
			continue; 	// FIXME(MED): partial RPC?

		portInfo[portIdx]->stopTransmit();
	}
	// TODO(LOW): fill-in response "Ack"????
	done->Run();
}

void MyService::startCapture(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::stopCapture(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::getCaptureBuffer(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::CaptureBufferList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::getStats(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::PortStatsList* response,
::google::protobuf::Closure* done)
{
	//qDebug("In %s", __PRETTY_FUNCTION__);

	for (int i=0; i < request->port_id_size(); i++)
	{
		uint 	portidx;
		::OstProto::PortStats	*s;

		portidx = request->port_id(i).id();
		if (portidx >= numPorts)
			continue; 	// FIXME(med): partial rpc?

		s = response->add_port_stats();
		s->mutable_port_id()->set_id(request->port_id(i).id());

		s->set_rx_pkts(portInfo[portidx]->stats.rxPkts -
				portInfo[portidx]->epochStats.rxPkts);
		s->set_rx_bytes(portInfo[portidx]->stats.rxBytes -
				portInfo[portidx]->epochStats.rxBytes);
		s->set_rx_pkts_nic(portInfo[portidx]->stats.rxPktsNic -
				portInfo[portidx]->epochStats.rxPktsNic);
		s->set_rx_bytes_nic(portInfo[portidx]->stats.rxBytesNic -
				portInfo[portidx]->epochStats.rxBytesNic);
		s->set_rx_pps(portInfo[portidx]->stats.rxPps);
		s->set_rx_bps(portInfo[portidx]->stats.rxBps);

		s->set_tx_pkts(portInfo[portidx]->stats.txPkts -
				portInfo[portidx]->epochStats.txPkts);
		s->set_tx_bytes(portInfo[portidx]->stats.txBytes -
				portInfo[portidx]->epochStats.txBytes);
		s->set_tx_pkts_nic(portInfo[portidx]->stats.txPktsNic -
				portInfo[portidx]->epochStats.txPktsNic);
		s->set_tx_bytes_nic(portInfo[portidx]->stats.txBytesNic -
				portInfo[portidx]->epochStats.txBytesNic);
		s->set_tx_pps(portInfo[portidx]->stats.txPps);
		s->set_tx_bps(portInfo[portidx]->stats.txBps);
	}

	done->Run();
}

void MyService::clearStats(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __PRETTY_FUNCTION__);

	for (int i=0; i < request->port_id_size(); i++)
	{
		uint portIdx;

		portIdx = request->port_id(i).id();
		if (portIdx >= numPorts)
			continue; 	// FIXME(MED): partial RPC?

		portInfo[portIdx]->resetStats();
	}
	// TODO(LOW): fill-in response "Ack"????

	done->Run();
}

