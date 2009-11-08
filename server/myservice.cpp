
#include <qglobal.h>
#include <qendian.h>
#include "qdebug.h"

#include "myservice.h"
#include "../common/protocollistiterator.h"
#include "../common/abstractprotocol.h"

#include "../rpc/pbrpccontroller.h"

#if 0
#include <pcap-int.h>
#include <Ntddndis.h>
#endif

#define LOG(...)	{sprintf(logStr, __VA_ARGS__); host->Log(logStr);}
#define MB			(1024*1024)

StreamInfo::StreamInfo()
{
}

StreamInfo::~StreamInfo()
{
}

int StreamInfo::makePacket(uchar *buf, int bufMaxSize, int n)
{
	int		pktLen, len = 0;

	pktLen = frameLen(n);

	// pktLen is adjusted for CRC/FCS which will be added by the NIC
	pktLen -=  4;

	if ((pktLen < 0) || (pktLen > bufMaxSize))
		return 0;

	ProtocolListIterator	*iter;

	iter = createProtocolListIterator();
	while (iter->hasNext())
	{
		AbstractProtocol	*proto;
		QByteArray			ba;

		proto = iter->next();
		ba = proto->protocolFrameValue(n);

		if (len + ba.size() < bufMaxSize)
			memcpy(buf+len, ba.constData(), ba.size());
		len += ba.size();
	}
	delete iter;

	return pktLen;
}


//
// ------------------ PortInfo --------------------
//
PortInfo::PortInfo(uint id, pcap_if_t *dev)
	: monitorRx(this), monitorTx(this), transmitter(this), capturer(this)
{
    char errbuf[PCAP_ERRBUF_SIZE];

	this->dev = dev;

#ifdef Q_OS_WIN32
	adapter = PacketOpenAdapter(dev->name);
	if (!adapter)
		qFatal("Unable to open adapter %s", dev->name);
	oidData = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) + sizeof(uint));
	if (oidData)
	{
		memset(oidData, 0, sizeof(PACKET_OID_DATA) + sizeof(uint));
		oidData->Length=sizeof(uint);
	}
	else
		qFatal("failed to alloc oidData");
#endif

	/*
	 * Get 2 device handles - one for rx and one for tx. If we use only
	 * one handle for both rx and tx anythin that we tx using the single
	 * handle is not received back to us
	 */
	devHandleRx = pcap_open_live(dev->name, 0, PCAP_OPENFLAG_PROMISCUOUS ,
		1000 /*ms*/, errbuf);
	if (devHandleRx == NULL)
	{
		qDebug("Error opening port %s: %s\n", 
				dev->name, pcap_geterr(devHandleRx));
	}

#if 0
	if (pcap_setdirection(devHandleRx, PCAP_D_IN)<0)
	{
		qDebug("[%s] Error setting direction inbound only\n", dev->name);
	}
#endif 

	/* By default, put the interface in statistics mode */
	if (pcap_setmode(devHandleRx, MODE_STAT)<0)
	{
		qDebug("Error setting statistics mode.\n");
	}

	devHandleTx = pcap_open_live(dev->name, 0, PCAP_OPENFLAG_PROMISCUOUS ,
		1000 /*ms*/, errbuf);
	if (devHandleTx == NULL)
	{
		qDebug("Error opening port %s: %s\n", 
				dev->name, pcap_geterr(devHandleTx));
	}

#if 0
	if (pcap_setdirection(devHandleTx, PCAP_D_OUT)<0)
	{
		qDebug("[%s] Error setting direction outbound only\n", dev->name);
	}
#endif

	/* By default, put the interface in statistics mode */
	if (pcap_setmode(devHandleTx, MODE_STAT)<0)
	{
		qDebug("Error setting statistics mode.\n");
	}

	d.mutable_port_id()->set_id(id);

#ifdef Q_OS_WIN32
	d.set_name(QString("if%1 ").arg(id).toAscii().constData());
#else
	if (dev->name)
		d.set_name(dev->name);
	else
		d.set_name(QString("if%1 ").arg(id).toAscii().constData());
#endif
	d.set_name(d.name()+"{"+
			pcap_datalink_val_to_name(pcap_datalink(devHandleRx))+"}");

	if (dev->description)
		d.set_description(dev->description);
	d.set_is_enabled(true);	//! \todo (LOW) admin enable/disable of port
	d.set_is_exclusive_control(false); //! \todo (HIGH) port exclusive control

	memset((void*) &stats, 0, sizeof(stats));
	resetStats();

	linkState = OstProto::LinkStateUnknown;

	// We'll create sendqueue later when required
	sendQueueList.clear();
	returnToQIdx = -1;
	pcapExtra.txPkts = 0;
	pcapExtra.txBytes = 0;
	isSendQueueDirty=true;

	// Start the monitor thread
	monitorRx.start();
	monitorTx.start();
}

void PortInfo::updateLinkState()
{
#ifdef Q_OS_WIN32
	OstProto::LinkState	newLinkState 
		= OstProto::LinkStateUnknown;

	memset(oidData, 0, sizeof(PACKET_OID_DATA) + sizeof(uint));
	oidData->Oid = OID_GEN_MEDIA_CONNECT_STATUS;
	oidData->Length = sizeof(uint);
	if (PacketRequest(adapter, 0, oidData))
	{
		uint state;

		if (oidData->Length == sizeof(state))
		{
			memcpy((void*)&state, (void*)oidData->Data, oidData->Length);
			if (state == 0)
				newLinkState = OstProto::LinkStateUp;
			else if (state == 1)
				newLinkState = OstProto::LinkStateDown;
		}
	}

	linkState = newLinkState;
#elif defined(Q_OS_LINUX)
	//! \todo (HI) implement link state for linux - get from /proc maybe?
#endif
}

void PortInfo::update()
{
	uchar				pktBuf[2000];
	pcap_pkthdr			pktHdr;
	ost_pcap_send_queue	sendQ;

	qDebug("In %s", __FUNCTION__);

	if (sendQueueList.size())
	{
		foreach(sendQ, sendQueueList)
			pcap_sendqueue_destroy(sendQ.sendQueue);
	}
	sendQueueList.clear();
	returnToQIdx = -1;

	//! \todo (LOW): calculate sendqueue size
	sendQ.sendQueue = pcap_sendqueue_alloc(1*MB);
	sendQ.sendQueueCumLen.clear();
	
	// First sort the streams by ordinalValue
	qSort(streamList);

	pktHdr.ts.tv_sec = 0;
	pktHdr.ts.tv_usec = 0;
	for (int i = 0; i < streamList.size(); i++)
	{
//_restart:
		if (streamList[i]->isEnabled())
		{
			long numPackets, numBursts;
			long ibg, ipg;

			switch (streamList[i]->sendUnit())
			{
			case OstProto::StreamControl::e_su_bursts:
				numBursts = streamList[i]->numBursts();
				numPackets = streamList[i]->burstSize();
				ibg = 1000000/streamList[i]->burstRate();
				ipg = 0;
				break;
			case OstProto::StreamControl::e_su_packets:
				numBursts = 1;
				numPackets = streamList[i]->numPackets();
				ibg = 0;
				ipg = 1000000/streamList[i]->packetRate();
				break;
			default:
				qWarning("Unhandled stream control unit %d",
					streamList[i]->sendUnit());
				continue;
			}
			qDebug("numBursts = %ld, numPackets = %ld\n",
					numBursts, numPackets);
			qDebug("ibg = %ld, ipg = %ld\n", ibg, ipg);

			for (int j = 0; j < numBursts; j++)
			{
				for (int k = 0; k < numPackets; k++)
				{
					int len;

					/*! \todo (HIGH) if pkt contents do not change across
					pkts then don't call makePacket(), rather reuse the 
					previous */
					len = streamList[i]->makePacket(pktBuf, sizeof(pktBuf), 
							j * numPackets + k);
					if (len > 0)
					{
						pktHdr.caplen = pktHdr.len = len;
						pktHdr.ts.tv_usec += ipg;
						if (pktHdr.ts.tv_usec > 1000000)
						{
							pktHdr.ts.tv_sec++;
							pktHdr.ts.tv_usec -= 1000000;
						}

						// Not enough space? Alloc another one!
						if ((sendQ.sendQueue->len + len + sizeof(pcap_pkthdr)) 
							 > sendQ.sendQueue->maxlen)
						{
							sendQueueList.append(sendQ);

							//! \todo (LOW): calculate sendqueue size
							sendQ.sendQueue = pcap_sendqueue_alloc(1*MB);
							sendQ.sendQueueCumLen.clear();

#if 0
							pktHdr.ts.tv_sec = 0;
							pktHdr.ts.tv_usec = 0;
#endif
						}

						qDebug("q(%d, %d, %d) sec = %lu usec = %lu",
								i, j, k, pktHdr.ts.tv_sec, pktHdr.ts.tv_usec);

						if (-1 == pcap_sendqueue_queue(sendQ.sendQueue, &pktHdr, 
									(u_char*) pktBuf))
						{
							qDebug("[port %d] sendqueue_queue() failed for "
									"streamidx %d\n", id(), i);
						}
						else
							sendQ.sendQueueCumLen.append(sendQ.sendQueue->len);
					}
				} // for (numPackets)
				pktHdr.ts.tv_usec += ibg;
				if (pktHdr.ts.tv_usec > 1000000)
				{
					pktHdr.ts.tv_sec++;
					pktHdr.ts.tv_usec -= 1000000;
				}
			} // for (numBursts)

			switch(streamList[i]->nextWhat())
			{
				case ::OstProto::StreamControl::e_nw_stop:
					goto _stop_no_more_pkts;

				case ::OstProto::StreamControl::e_nw_goto_id:
					/*! \todo (MED): define and use 
					streamList[i].d.control().goto_stream_id(); */

					/*! \todo (MED): assumes goto Id is less than current!!!! 
					 To support goto to any id, do
					 if goto_id > curr_id then 
					     i = goto_id;
					     goto restart;
				     else
					     returnToQIdx = 0;
					 */

					returnToQIdx=0;
					goto _stop_no_more_pkts;

				case ::OstProto::StreamControl::e_nw_goto_next:
					break;

				default:
					qFatal("---------- %s: Unhandled case (%d) -----------",
							__FUNCTION__, streamList[i]->nextWhat() );
					break;
			}

		} // if (stream is enabled)
	} // for (numStreams)

_stop_no_more_pkts:
	// The last alloc'ed sendQ appended here
	sendQueueList.append(sendQ);

	isSendQueueDirty = false;
}

void PortInfo::startTransmit()
{
	transmitter.start();
}

void PortInfo::stopTransmit()
{
	transmitter.stop();
}

void PortInfo::startCapture()
{
	capturer.start();
}

void PortInfo::stopCapture()
{
	capturer.stop();
}

QFile* PortInfo::captureFile()
{
	return capturer.captureFile();
}

void PortInfo::resetStats()
{
	memcpy((void*) &epochStats, (void*) &stats, sizeof(stats));
}

//
// ------------------ PortMonitor -------------------
//

PortInfo::PortMonitorRx::PortMonitorRx(PortInfo *port)
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

PortInfo::PortMonitorTx::PortMonitorTx(PortInfo *port)
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

#ifdef Q_OS_WIN32
void PortInfo::PortMonitorRx::callbackRx(u_char *state, 
		const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	// This is the WinPcap Callback - which is a 'stats mode' callback

	uint		usec;
	PortInfo	*port = (PortInfo*) state;

	quint64 pkts;
	quint64 bytes;

	// Update RxStats and RxRates using PCAP data
	pkts  = *((quint64*)(pkt_data + 0));
	bytes = *((quint64*)(pkt_data + 8));

#if 0
	if (port->id() == 2)
		qDebug("# %llu", pkts);
#endif

	// Note: PCAP reported bytes includes ETH_FRAME_HDR_SIZE - adjust for it
	bytes -= pkts * ETH_FRAME_HDR_SIZE;

	usec = (header->ts.tv_sec - port->lastTsRx.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTsRx.tv_usec);
	port->stats.rxPps = (pkts * 1000000) / usec;
	port->stats.rxBps = (bytes * 1000000) / usec;

	port->stats.rxPkts += pkts;
	port->stats.rxBytes += bytes;

	// Store curr timestamp as last timestamp
	port->lastTsRx.tv_sec = header->ts.tv_sec;
	port->lastTsRx.tv_usec = header->ts.tv_usec;

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
#if 0
	port->monitorRx.oidData->Oid = OID_GEN_RCV_OK;
	if (PacketRequest(port->devHandleRx->adapter, 0, port->monitorRx.oidData))
	{
		if (port->monitorRx.oidData->Length <= sizeof(port->stats.rxPktsNic))
			memcpy((void*)&port->stats.rxPktsNic,
					(void*)port->monitorRx.oidData->Data, 
					port->monitorRx.oidData->Length);
	}
#endif
}
void PortInfo::PortMonitorTx::callbackTx(u_char *state, 
		const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	// This is the WinPcap Callback - which is a 'stats mode' callback

	uint		usec;
	PortInfo	*port = (PortInfo*) state;

	quint64 pkts;
	quint64 bytes;


#if 0
	// Update RxStats and RxRates using PCAP data
	pkts  = *((quint64*)(pkt_data + 0));
	bytes = *((quint64*)(pkt_data + 8));

#if 0
	if (port->id() == 2)
		qDebug("@ %llu", pkts);
#endif

	// Note: PCAP reported bytes includes ETH_FRAME_HDR_SIZE - adjust for it
	bytes -= pkts * ETH_FRAME_HDR_SIZE;

	usec = (header->ts.tv_sec - port->lastTsTx.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTsTx.tv_usec);
	port->stats.txPps = (pkts * 1000000) / usec;
	port->stats.txBps = (bytes * 1000000) / usec;

	port->stats.txPkts += pkts;
	port->stats.txBytes += bytes;
#endif

	// Since WinPCAP (due to NDIS limitation) cannot distinguish between
	// rx/tx packets, pcap stats are not of much use - for the tx stats
	// update from PcapExtra

	pkts  = port->pcapExtra.txPkts - port->stats.txPkts;
	bytes  = port->pcapExtra.txBytes - port->stats.txBytes;

	// Use the pcap timestamp for rate calculation though
	usec = (header->ts.tv_sec - port->lastTsTx.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTsTx.tv_usec);
	port->stats.txPps = (pkts * 1000000) / usec;
	port->stats.txBps = (bytes * 1000000) / usec;

	port->stats.txPkts = port->pcapExtra.txPkts;
	port->stats.txBytes = port->pcapExtra.txBytes;

	// Store curr timestamp as last timestamp
	port->lastTsTx.tv_sec = header->ts.tv_sec;
	port->lastTsTx.tv_usec = header->ts.tv_usec;

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
#if 0
	port->monitorTx.oidData->Oid = OID_GEN_XMIT_OK;
	if (PacketRequest(port->devHandleTx->adapter, 0, port->monitorTx.oidData))
	{
		if (port->monitorTx.oidData->Length <= sizeof(port->stats.txPktsNic))
			memcpy((void*)&port->stats.txPktsNic,
					(void*)port->monitorTx.oidData->Data, 
					port->monitorTx.oidData->Length);
	}
#endif
}
#else
void PortInfo::PortMonitorRx::callbackRx(u_char *state, 
		const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	// This is the LibPcap Callback - which is a 'capture mode' callback
	// This callback is called once for EVERY packet

	uint		usec;
	PortInfo	*port = (PortInfo*) state;

	quint64 pkts;
	quint64 bytes;

	// Update RxStats and RxRates using PCAP data
	usec = (header->ts.tv_sec - port->lastTsRx.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTsRx.tv_usec);
	//! \todo support Rx Pkt/Bit rate on Linux (libpcap callback)
#if 0
	port->stats.rxPps = (pkts * 1000000) / usec;
	port->stats.rxBps = (bytes * 1000000) / usec;
#endif

	// Note: For a 'capture callback' PCAP reported bytes DOES NOT include
	// ETH_FRAME_HDR_SIZE - so don't adjust for it
	port->stats.rxPkts++;
	port->stats.rxBytes += header->len;

	// Store curr timestamp as last timestamp
	port->lastTsRx.tv_sec = header->ts.tv_sec;
	port->lastTsRx.tv_usec = header->ts.tv_usec;
}

void PortInfo::PortMonitorTx::callbackTx(u_char *state, 
		const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	// This is the LibPcap Callback - which is a 'capture mode' callback
	// This callback is called once for EVERY packet

	uint		usec;
	PortInfo	*port = (PortInfo*) state;

	quint64 pkts;
	quint64 bytes;

	// Update TxStats and TxRates using PCAP data
	usec = (header->ts.tv_sec - port->lastTsTx.tv_sec) * 1000000 + 
		(header->ts.tv_usec - port->lastTsTx.tv_usec);
	//! \todo support Tx Pkt/Bit rate on Linux (libpcap callback)
#if 0
	port->stats.txPps = (pkts * 1000000) / usec;
	port->stats.txBps = (bytes * 1000000) / usec;
#endif

	// Note: For a 'capture callback' PCAP reported bytes DOES NOT include
	// ETH_FRAME_HDR_SIZE - so don't adjust for it

	port->stats.txPkts++;
	port->stats.txBytes += header->len;

	// Store curr timestamp as last timestamp
	port->lastTsTx.tv_sec = header->ts.tv_sec;
	port->lastTsTx.tv_usec = header->ts.tv_usec;
}
#endif
void PortInfo::PortMonitorRx::run()
{
	int		ret;

	qDebug("before pcap_loop rx \n");
#if 1
	/* Start the main loop */
	ret = pcap_loop(port->devHandleRx, -1,
			&PortInfo::PortMonitorRx::callbackRx, (u_char*) port);

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
#else
	while (1)
	{
		/* Start the main loop */
		ret = pcap_dispatch(port->devHandleRx, -1,
				&PortInfo::PortMonitorRx::callbackRx, (u_char*) port);

		switch(ret)
		{
			case -1:
				qDebug("Unsolicited (error) return from pcap_loop() %s\n",
						pcap_geterr(port->devHandleRx));
				break;
			case -2:
				qDebug("Solicited return from pcap_loop()\n");
				break;
			default:
				//qDebug("%d pkts rcvd\n", ret);
				break;
		}
	}
#endif
}

void PortInfo::PortMonitorTx::run()
{
	int		ret;

	qDebug("before pcap_loopTx\n");
#if 1
	/* Start the main loop */
	ret = pcap_loop(port->devHandleTx, -1,
			&PortInfo::PortMonitorTx::callbackTx, (u_char*) port);

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
#else
	while (1)
	{
		/* Start the main loop */
		ret = pcap_dispatch(port->devHandleTx, -1,
				&PortInfo::PortMonitorTx::callbackTx, (u_char*) port);

		switch(ret)
		{
			case -1:
				qDebug("Unsolicited (error) return from pcap_loop() %s\n",
						pcap_geterr(port->devHandleTx));
				break;
			case -2:
				qDebug("Solicited return from pcap_loop()\n");
				break;
			default:
				//qDebug("%d pkts rcvd\n", ret);
				break;
		}
	}
#endif
}

/*--------------- PortTransmitter ---------------*/

PortInfo::PortTransmitter::PortTransmitter(PortInfo *port)
{
	this->port = port;
}

void PortInfo::PortTransmitter::run()
{
	//! \todo (MED) Stream Mode - continuous: define before implement

	// NOTE1: We can't use pcap_sendqueue_transmit() directly even on Win32
	// 'coz of 2 reasons - there's no way of stopping it before all packets
	// in the sendQueue are sent out and secondly, stats are available only
	// when all packets have been sent - no periodic updates
	// 
	// NOTE2: Transmit on the Rx Handle so that we can receive it back
	// on the Tx Handle to do stats
	//
	// NOTE3: Update pcapExtra counters - port TxStats will be updated in the
	// 'stats callback' function so that both Rx and Tx stats are updated
	// together

	m_stop = 0;
	ost_pcap_sendqueue_list_transmit(port->devHandleRx, port->sendQueueList, 
		port->returnToQIdx, true, &m_stop,
		&port->pcapExtra.txPkts, &port->pcapExtra.txBytes,
		QThread::usleep);
	m_stop = 0;
}

void PortInfo::PortTransmitter::stop()
{
	m_stop = 1;
}

/*--------------- PortCapture ---------------*/

PortInfo::PortCapture::PortCapture(PortInfo *port)
{
	this->port = port;
	capHandle = NULL;
	dumpHandle = NULL;
}

PortInfo::PortCapture::~PortCapture()
{
}

void PortInfo::PortCapture::run()
{
	int ret;
	char errbuf[PCAP_ERRBUF_SIZE];

	capHandle = pcap_open_live(port->dev->name, 65535, 
					PCAP_OPENFLAG_PROMISCUOUS, 1000 /* ms */, errbuf);
	if (capHandle == NULL)
	{
		qDebug("Error opening port %s: %s\n", 
				port->dev->name, pcap_geterr(capHandle));
		return;
	}

	if (!capFile.isOpen())
	{
		if (!capFile.open())
			qFatal("Unable to open temp cap file");
		return;
	}

	qDebug("cap file = %s", capFile.fileName().toAscii().constData());
	dumpHandle = pcap_dump_open(capHandle, 
		capFile.fileName().toAscii().constData());

	m_stop = 0;
	while (m_stop == 0)
	{
		struct pcap_pkthdr *hdr;
		const uchar *data;

		ret = pcap_next_ex(capHandle, &hdr, &data);
		switch (ret)
		{
			case 1:
				pcap_dump((uchar*) dumpHandle, hdr, data);
			case 0:
				continue;
			case -1:
				qWarning("%s: error reading packet (%d): %s", 
						__PRETTY_FUNCTION__, ret, pcap_geterr(capHandle));
				break;
			case -2:
			default:
				qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
		}
	}
	m_stop = 0;
	pcap_dump_close(dumpHandle);
	pcap_close(capHandle);
	dumpHandle = NULL;
	capHandle = NULL;
}

void PortInfo::PortCapture::stop()
{
	m_stop = 1;
}

QFile* PortInfo::PortCapture::captureFile()
{
	return &capFile;
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
		if (streamId == portInfo[portIdx]->streamList.at(i)->mStreamId.id())
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
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
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
		goto _exit;		//! \todo (LOW): Partial status of RPC
	}

	response->mutable_port_id()->set_id(portIdx);
	for (int j = 0; j < portInfo[portIdx]->streamList.size(); j++)
	{
		OstProto::StreamId	*s;

		s = response->add_stream_id();
		s->CopyFrom(portInfo[portIdx]->streamList[j]->mStreamId);
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
			continue;		//! \todo(LOW): Partial status of RPC

		s = response->add_stream();

		portInfo[portIdx]->streamList[streamIndex]->protoDataCopyInto(*s);
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
		StreamInfo		*s = new StreamInfo;

		// If stream with same id as in request exists already ==> error!!
		streamIndex = getStreamIndex(portIdx, request->stream_id(i).id());
		if (streamIndex >= 0)
			continue;		//! \todo (LOW): Partial status of RPC

		// Append a new "default" stream - actual contents of the new stream is
		// expected in a subsequent "modifyStream" request - set the stream id
		// now itself however!!!
		s->mStreamId.CopyFrom(request->stream_id(i));
		portInfo[portIdx]->streamList.append(s);

		//! \todo (LOW): fill-in response "Ack"????
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
			continue;		//! \todo (LOW): Partial status of RPC

		delete portInfo[portIdx]->streamList.takeAt(streamIndex);

		//! \todo (LOW): fill-in response "Ack"????
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
			continue;		//! \todo (LOW): Partial status of RPC

		portInfo[portIdx]->streamList[streamIndex]->protoDataCopyFrom(
			request->stream(i));

		//! \todo(LOW): fill-in response "Ack"????
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
			continue; 	//! \todo (LOW): partial RPC?

		if (portInfo[portIdx]->isDirty())
			portInfo[portIdx]->update();
	}

	for (int i=0; i < request->port_id_size(); i++)
	{
		uint portIdx;

		portIdx = request->port_id(i).id();
		if (portIdx >= numPorts)
			continue; 	//! \todo (LOW): partial RPC?

		portInfo[portIdx]->startTransmit();
	}

	//! \todo (LOW): fill-in response "Ack"????

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
			continue; 	//! \todo (LOW): partial RPC?

		portInfo[portIdx]->stopTransmit();
	}
	//! \todo (LOW): fill-in response "Ack"????
	done->Run();
}

void MyService::startCapture(::google::protobuf::RpcController* controller,
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
			continue; 	//! \todo (LOW): partial RPC?

		portInfo[portIdx]->startCapture();
	}

	done->Run();
}

void MyService::stopCapture(::google::protobuf::RpcController* controller,
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
			continue; 	//! \todo (LOW): partial RPC?

		portInfo[portIdx]->stopCapture();
	}

	done->Run();
}

void MyService::getCaptureBuffer(::google::protobuf::RpcController* controller,
const ::OstProto::PortId* request,
::OstProto::CaptureBuffer* response,
::google::protobuf::Closure* done)
{
	uint portIdx;
	qDebug("In %s", __PRETTY_FUNCTION__);

	portIdx = request->id();
	if (portIdx >= numPorts)
	{
		controller->SetFailed("invalid portid");
		goto _exit;
	}

	portInfo[portIdx]->stopCapture();
	static_cast<PbRpcController*>(controller)->setBinaryBlob(
		portInfo[portIdx]->captureFile());

_exit:
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
		OstProto::PortState		*st;

		portidx = request->port_id(i).id();
		if (portidx >= numPorts)
			continue; 	//! \todo(LOW): partial rpc?

		s = response->add_port_stats();
		s->mutable_port_id()->set_id(request->port_id(i).id());

#if 0
		if (portidx == 2)
		{
			qDebug("<%llu", portInfo[portidx]->epochStats.rxPkts);
			qDebug(">%llu", portInfo[portidx]->stats.rxPkts);
		}
#endif

		portInfo[portidx]->updateLinkState(); 

		st = s->mutable_state(); 
		st->set_link_state(portInfo[portidx]->linkState); 
		st->set_is_transmit_on(portInfo[portidx]->transmitter.isRunning()); 
		st->set_is_capture_on(portInfo[portidx]->capturer.isRunning()); 

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
			continue; 	//! \todo (LOW): partial RPC?

		portInfo[portIdx]->resetStats();
	}
	//! \todo (LOW): fill-in response "Ack"????

	done->Run();
}

