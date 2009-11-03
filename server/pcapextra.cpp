#include <string.h>	// memcpy()
#include <stdlib.h> // malloc(), free()
#include "pcapextra.h"

/* NOTE: All code borrowed from WinPcap */

#ifndef Q_OS_WIN32
int pcap_setmode(pcap_t *p, int mode)
{
	// no STAT mode in libpcap, so just return 0 to indicate success
	return 0;
}

pcap_send_queue* pcap_sendqueue_alloc (u_int memsize)
{
	pcap_send_queue *tqueue;

	/* Allocate the queue */
	tqueue = (pcap_send_queue*)malloc(sizeof(pcap_send_queue));
	if(tqueue == NULL){
		return NULL;
	}

	/* Allocate the buffer */
	tqueue->buffer = (char*)malloc(memsize);
	if(tqueue->buffer == NULL){
		free(tqueue);
		return NULL;
	}

	tqueue->maxlen = memsize;
	tqueue->len = 0;

	return tqueue;
}

void pcap_sendqueue_destroy (pcap_send_queue *queue)
{
	free(queue->buffer);
	free(queue);
}

int pcap_sendqueue_queue (pcap_send_queue *queue,
		        const struct pcap_pkthdr *pkt_header, const u_char *pkt_data)
{
	if(queue->len + sizeof(struct pcap_pkthdr) + pkt_header->caplen > 
			queue->maxlen)
	{
		return -1;
	}

	/* Copy the pcap_pkthdr header*/
	memcpy(queue->buffer + queue->len, pkt_header, sizeof(struct pcap_pkthdr));
	queue->len += sizeof(struct pcap_pkthdr);

	/* copy the packet */
	memcpy(queue->buffer + queue->len, pkt_data, pkt_header->caplen);
	queue->len += pkt_header->caplen;

	return 0;
}
#endif

u_int ost_pcap_sendqueue_list_transmit(pcap_t *p, 
		QList<ost_pcap_send_queue> sendQueueList, int returnToQIdx, int sync,
		int *p_stop, quint64* p_pkts, quint64* p_bytes,
		void (*pf_usleep)(ulong))
{
	uint i, ret = 0;
	ost_pcap_send_queue sq;

	for(i = 0; i < sendQueueList.size(); i++)
	{
_restart:
		sq = sendQueueList.at(i);
		ret += ost_pcap_sendqueue_transmit(p, sq.sendQueue, sync,
				p_stop, p_pkts, p_bytes, pf_usleep);

		if (*p_stop)
			return ret;

		//! \todo (HIGH): Timing between subsequent sendQueues
	}

	if (returnToQIdx >= 0)
	{
		i = returnToQIdx;

		//! \todo (HIGH) 1s fixed; Change this to ipg of last stream
		(*pf_usleep)(1000000);
		goto _restart;
	}

	return ret;
}

u_int ost_pcap_sendqueue_transmit(pcap_t *p,
		pcap_send_queue *queue, int sync,
		int *p_stop, quint64* p_pkts, quint64* p_bytes,
		void (*pf_usleep)(ulong))
{
	char*	PacketBuff = queue->buffer;
	int		Size = queue->len;

    struct	pcap_pkthdr *winpcap_hdr;
	struct  timeval		ts;
    char*	EndOfUserBuff = (char *)PacketBuff + Size;
    int		ret;

    // Start from the first packet
    winpcap_hdr = (struct pcap_pkthdr*)PacketBuff;

    if((char*)winpcap_hdr + winpcap_hdr->caplen + sizeof(struct pcap_pkthdr) >
		EndOfUserBuff )
    {
        // Malformed buffer
        return 0;
    }

	if (sync)
		ts = winpcap_hdr->ts;

    while( true ){

		if (*p_stop)
			return (char*)winpcap_hdr - (char*)PacketBuff;

        if(winpcap_hdr->caplen ==0 || winpcap_hdr->caplen > 65536)
        {
            // Malformed header
            return 0;
        }

        // Send the packet
		ret = pcap_sendpacket(p, 
				(unsigned char*)winpcap_hdr + sizeof(struct pcap_pkthdr), 
				winpcap_hdr->caplen);

        if(ret < 0){
            // Error sending the packet
            return (char*)winpcap_hdr - (char*)PacketBuff;
        }

		if (p_pkts) (*p_pkts)++;
		if (p_bytes) (*p_bytes) += winpcap_hdr->caplen;

        // Step to the next packet in the buffer
        //(char*)winpcap_hdr += winpcap_hdr->caplen + sizeof(struct pcap_pkthdr);
        winpcap_hdr = (struct pcap_pkthdr*) ((char*)winpcap_hdr + 
				winpcap_hdr->caplen + sizeof(struct pcap_pkthdr));

        // Check if the end of the user buffer has been reached
        if( (char*)winpcap_hdr >= EndOfUserBuff )
        {
                return (char*)winpcap_hdr - (char*)PacketBuff;
        }

		if (sync)
		{
			long usec = (winpcap_hdr->ts.tv_sec-ts.tv_sec)*1000000 +
				(winpcap_hdr->ts.tv_usec - ts.tv_usec);

			if (usec)
			{
				(*pf_usleep)(usec);
				ts = winpcap_hdr->ts;
			}
		}
    }
}

