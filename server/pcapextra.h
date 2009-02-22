#ifndef _PCAP_EXTRA_H
#define _PCAP_EXTRA_H

#include <Qt>
#include <QList>

#include "pcap.h"

struct ost_pcap_send_queue
{
	pcap_send_queue	*sendQueue;
	//! Used to track num of packets (and their sizes) in the
	// send queue. Also used to find out actual num of pkts sent 
	// in case of partial send in pcap_sendqueue_transmit()
	QList<uint> 	sendQueueCumLen;
};

// Common for all OS - *nix or Win32
u_int ost_pcap_sendqueue_list_transmit(pcap_t *p, 
		QList<ost_pcap_send_queue> sendQueueList, int sync,
		int *p_stop, quint64* p_pkts, quint64* p_bytes,
		void (*pf_usleep)(ulong));

u_int ost_pcap_sendqueue_transmit (pcap_t *p,
		pcap_send_queue *queue, int sync,
		int *p_stop, quint64* p_pkts, quint64* p_bytes,
		void (*pf_usleep)(ulong));

#ifndef Q_OS_WIN32
// Only for non Win32


#define PCAP_OPENFLAG_PROMISCUOUS	1

struct pcap_send_queue
{
	u_int maxlen;       
	u_int len;          
	char *buffer;  
};

int	pcap_setmode(pcap_t *p, int mode);
#define MODE_STAT   1

pcap_send_queue* pcap_sendqueue_alloc (u_int memsize);
void pcap_sendqueue_destroy (pcap_send_queue *queue);
int pcap_sendqueue_queue (pcap_send_queue *queue, 
		const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);


#endif

#endif

