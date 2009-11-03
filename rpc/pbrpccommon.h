#ifndef _PB_RPC_COMMON_H
#define _PB_RPC_COMMON_H

//! \todo (LOW) check which one is right - wrong one seems to be working!!!!!
#if 0
#define GET16(p)	(quint16)(	\
	  (*((quint8*)(p)+0) << 8 )	\
	| (*((quint8*)(p)+1)))
#else
#define GET16(p)	(quint16)(	\
	  (*((quint8*)(p)+1) << 8 )	\
	| (*((quint8*)(p)+0)))
#define GET32(p)	(quint32)(		\
	  (*((quint8*)(p)+3) << 24)	\
	| (*((quint8*)(p)+2) << 16)	\
	| (*((quint8*)(p)+1) << 8 )		\
	| (*((quint8*)(p)+0)))
#endif

#define BYTESWAP4(x) \
  (((x & 0xFF000000) >> 24) | \
   ((x & 0x00FF0000) >> 8) | \
   ((x & 0x0000FF00) << 8)  | \
   ((x & 0x000000FF) << 24))

#define BYTESWAP2(x) \
  (((x & 0xFF00) >> 8) | \
   ((x & 0x00FF) << 8))

//! \todo (LOW) : portability
#if 1
#define HTONL(x)	BYTESWAP4(x)
#define NTOHL(x)	BYTESWAP4(x)
#define HTONS(x)	BYTESWAP2(x)
#define NTOHS(x)	BYTESWAP2(x)
#else
#define HTONL(x)	(x)
#define NTOHL(x)	(x)
#define HTONS(x)	(x)
#define NTOHS(x)	(x)
#endif

// Print a HexDump
#define BUFDUMP(ptr, len) qDebug("%s", QString(QByteArray((char*)(ptr), \
	(len)).toHex()).toAscii().data()); 

/*
** RPC Header (8)
**	- MSG_TYPE (2)
**	- METHOD_ID (2)
**	- LEN (4) [not including this header]
*/
#define PB_HDR_SIZE				8

#define PB_MSG_TYPE_REQUEST		1
#define PB_MSG_TYPE_RESPONSE	2
#define PB_MSG_TYPE_BINBLOB		3

#define MSGBUF_SIZE		4096

#endif
