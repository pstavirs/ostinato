#include <string.h>	// memcpy()
#include <stdlib.h> // malloc(), free()
#include "pcapextra.h"

/* NOTE: All code borrowed from WinPcap */

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

u_int pcap_sendqueue_transmit (pcap_t *p, pcap_send_queue *queue, int sync)
{
	char*	PacketBuff = queue->buffer;
	int		Size = queue->len;

    struct	pcap_pkthdr *winpcap_hdr;
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

    while( true ){

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

        // Step to the next packet in the buffer
        //(char*)winpcap_hdr += winpcap_hdr->caplen + sizeof(struct pcap_pkthdr);
        winpcap_hdr = (struct pcap_pkthdr*) ((char*)winpcap_hdr + 
				winpcap_hdr->caplen + sizeof(struct pcap_pkthdr));

        // Check if the end of the user buffer has been reached
        if( (char*)winpcap_hdr >= EndOfUserBuff )
        {
                return (char*)winpcap_hdr - (char*)PacketBuff;
        }
    }
}

