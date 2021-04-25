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

#include "pcapextra.h"

#include <string.h>    // memcpy()
#include <stdlib.h> // malloc(), free()

/* NOTE: All code borrowed from WinPcap */

#ifdef Q_OS_WIN32

const char* pcapServiceStatus(const wchar_t* name)
{
    SC_HANDLE scm, svc;
    SERVICE_STATUS_PROCESS svcStatus;
    DWORD size;
    BOOL result;
    const char *status = "unknown";

    scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if(!scm)
        goto _exit;

    svc = OpenService(scm, name, SERVICE_QUERY_STATUS);
    if(!svc)
        goto _close_exit;

    result = QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&svcStatus), sizeof(svcStatus),
                &size);

    if(result == 0)
        goto _close_exit;

    switch(svcStatus.dwCurrentState) {
        case SERVICE_CONTINUE_PENDING:
            status = "continue pending";
            break;
        case SERVICE_PAUSE_PENDING:
            status = "pause pending";
            break;
        case SERVICE_PAUSED:
            status = "paused";
            break;
        case SERVICE_RUNNING:
            status = "running";
            break;
        case SERVICE_START_PENDING:
            status = "start pending";
            break;
        case SERVICE_STOP_PENDING:
            status = "stop pending";
            break;
        case SERVICE_STOPPED:
            status = "stopped";
            break;
    }

_close_exit:
    if (svc)
        CloseServiceHandle(svc);
    if (scm)
        CloseServiceHandle(scm);
_exit:
    return status;
}

#else // non-Windows

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


