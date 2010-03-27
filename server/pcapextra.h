/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _PCAP_EXTRA_H
#define _PCAP_EXTRA_H

#include <pcap.h>
#include <QtGlobal>

#ifndef Q_OS_WIN32

#define PCAP_OPENFLAG_PROMISCUOUS    1

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

