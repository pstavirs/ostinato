#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#define UINT8	unsigned char
#define UINT16	unsigned short
#define UINT32	unsigned int

#define BYTESWAP4(x) \
  (((x & 0xFF000000) >> 24) | \
   ((x & 0x00FF0000) >> 8) | \
   ((x & 0x0000FF00) << 8)  | \
   ((x & 0x000000FF) << 24))

#define BYTESWAP2(x) \
  (((x & 0xFF00) >> 8) | \
   ((x & 0x00FF) << 8))

// TODO: portability
#define HTONL(x)	BYTESWAP4(x)
#define NTOHL(x)	BYTESWAP4(x)
#define HTONS(x)	BYTESWAP2(x)
#define NTOHS(x)	BYTESWAP2(x)


typedef struct {
	UINT8	ver;
	UINT8	resv1;
	UINT16	resv2;
	UINT16 msgType;
	UINT16 msgLen;
} tCommHdr;

typedef enum {
	e_MT_CapabilityReq=1,
	e_MT_CapabilityInfo
} eMsgType;

typedef enum {
	e_TT_PortCapability
} eTlvType;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	port;
	UINT32	speed;
#define TLV_MAX_PORT_NAME	64
#define TLV_MAX_PORT_DESC	64
	char	name[TLV_MAX_PORT_NAME];
	char	desc[TLV_MAX_PORT_DESC];
} tTlvPortCapability;

#endif
