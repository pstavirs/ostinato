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
	e_MT_GetCapability=1,	// C-->S
	e_MT_CapabilityInfo,	// C<--S

	e_MT_ChangePortConfig,	// C-->S
	e_MT_GetPortConfig,		// C-->S
	e_MT_PortInfo,			// C<--S

	e_MT_StartTx,			// C-->S
	e_MT_StopTx,			// C-->S
	e_MT_StartCapture,		// C-->S
	e_MT_StopCapture,		// C-->S
	e_MT_GetCaptureBuffer,	// C-->S
	e_MT_CaptureBufferInfo,	// C-->S

	e_MT_GetStats,			// C-->S
	e_MT_StatsInfo,			// C<--S
	e_MT_ClearStats,		// C-->S

} eMsgType;

typedef enum {
	e_TT_PortCapability=0x0000,

	e_TT_StreamOper = 0x0100,	
	e_TT_StreamName,
	e_TT_StreamStatus,
	e_TT_StreamFrameLength,
	e_TT_StreamDataPattern,
	e_TT_StreamHeaderData,
} eTlvType;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
} tTlv;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	portSpeed;
#define TLV_MAX_PORT_NAME	64
#define TLV_MAX_PORT_DESC	64
	char	portName[TLV_MAX_PORT_NAME];
	char	portDesc[TLV_MAX_PORT_DESC];
} tTlvPortCapability;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
} tTlvStream;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	UINT16	rsvd;
	UINT16	streamOper;
#define TLV_STREAM_OPER_INSERT_HEAD		0x0001
#define TLV_STREAM_OPER_INSERT_TAIL		0x0002
#define TLV_STREAM_OPER_INSERT_BEFORE	0x0003
#define TLV_STREAM_OPER_DELETE			0x0010
	UINT32	StreamId;
} tTlvStreamOper;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	char	streamName[0];
} tTlvStreamName;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	UINT32	streamStatus;
#define TLV_STREAM_STATUS_DISABLED	0
#define TLV_STREAM_STATUS_ENABLED	1
} tTlvStreamStatus;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	UINT16	frameLenMode;
#define TLV_STREAM_FRAME_LEN_MODE_FIXED		0x0000
#define TLV_STREAM_FRAME_LEN_MODE_RANDOM	0x0001
#define TLV_STREAM_FRAME_LEN_MODE_INCREMENT	0x0002
#define TLV_STREAM_FRAME_LEN_MODE_DECREMENT	0x0003
	UINT16	frameLen;
	UINT16	frameLenMin;
	UINT16	frameLenMax;
} tTlvStreamFrameLength;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	UINT16	dataPatternMode;
#define TLV_STREAM_DATA_PATTERN_MODE_FIXED		0x0000
#define TLV_STREAM_DATA_PATTERN_MODE_RANDOM		0x0001
#define TLV_STREAM_DATA_PATTERN_MODE_INCREMENT	0x0002
#define TLV_STREAM_DATA_PATTERN_MODE_DECREMENT	0x0003
	UINT16	rsvd;
	UINT32	dataPattern;
} tTlvStreamDataPattern;

typedef struct {
	UINT16	tlvType;
	UINT16	tlvLen;
	UINT32	portId;
	UINT32	streamId;
	UINT16	rsvd;
	UINT16	headerLen;
	UINT8	header[0];
} tTlvStreamHeaderData;

typedef union {
	tTlvStream				tlv;
	tTlvStreamOper			oper;
	tTlvStreamName			name;
	tTlvStreamStatus		status;
	tTlvStreamFrameLength	frameLen;
	tTlvStreamDataPattern	dataPattern;
	tTlvStreamHeaderData	headerData;
} uTlvStream;

#endif
