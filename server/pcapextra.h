#ifndef _PCAP_EXTRA_H
#define _PCAP_EXTRA_H

#include <pcap.h>
#include <QtGlobal>

#ifndef Q_OS_WIN32

//#define PCAP_OPENFLAG_PROMISCUOUS    1

struct pcap_send_queue
{
    u_int maxlen;       
    u_int len;          
    char *buffer;  
};

pcap_send_queue* pcap_sendqueue_alloc (u_int memsize);
void pcap_sendqueue_destroy (pcap_send_queue *queue);
int pcap_sendqueue_queue (pcap_send_queue *queue, 
        const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);

#endif

#endif

