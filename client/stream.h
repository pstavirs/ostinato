#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>
#include <QString>

class StreamConfigDialog;
class StreamModel;
class PacketModel;

class Stream {

	friend class StreamConfigDialog;
	friend class StreamModel;
	friend class PacketModel;

	enum FrameType {
		e_ft_none,
		e_ft_eth_2,
		e_ft_802_3_raw,
		e_ft_802_3_llc,
		e_ft_snap
	};

	enum DataPatternMode {
		e_dp_fixed,
		e_dp_inc,
		e_dp_dec,
		e_dp_random
	};

	enum FrameLengthMode {
		e_fl_fixed,
		e_fl_inc,
		e_fl_dec,
		e_fl_random
	};

	enum MacAddrMode {
		e_mm_fixed,
		e_mm_inc,
		e_mm_dec,
	};

	enum IpAddrMode {
		e_im_fixed,
		e_im_inc_host,
		e_im_dec_host,
		e_im_random_host
	};

	// Meta Data
	struct {
		// Data Pattern
		DataPatternMode	patternMode;
		quint32			pattern;
		quint16			dataStartOfs;

		// Frame Length (includes CRC)
		FrameLengthMode	lenMode;
		quint16			frameLen;
		quint16			frameLenMin;
		quint16			frameLenMax;
	} meta;

	// Protocols
	struct {
		FrameType	ft;

		quint8		dsap;
		quint8		ssap;
		quint8		ctl;
		quint8		ouiMsb;
		quint16		ouiLshw;

		quint16		protoMask;
#define PM_L3_PROTO_NONE	0x0001
#define PM_L3_PROTO_OTHER	0x0002
#define PM_L4_PROTO_NONE	0x0004
#define PM_L4_PROTO_OTHER	0x0008

		quint16		etherType;
#define	ETH_TYP_IP		0x0800
#define	ETH_TYP_ARP		0x0806

		quint16		ipProto;
#define IP_PROTO_ICMP	0x01
#define IP_PROTO_IGMP	0x02
#define IP_PROTO_TCP	0x06
#define IP_PROTO_UDP	0x11
	} proto;

	// L2
	struct {
		// Ethernet
		struct {
			// Dst Mac
			quint16	dstMacMshw;
			quint32	dstMacLsw;
			MacAddrMode	dstMacMode;
			quint16	dstMacCount;
			quint16	dstMacStep;

			// srcMac
			quint16	srcMacMshw;
			quint32	srcMacLsw;
			MacAddrMode	srcMacMode;
			quint16	srcMacCount;
			quint16	srcMacStep;


			quint16	vlanMask;
#define VM_UNTAGGED				0x0000
#define VM_CVLAN_TAGGED			0x0001
#define VM_CVLAN_TPID_OVERRIDE	0x0002
#define VM_SVLAN_TAGGED			0x0100
#define VM_SVLAN_TPID_OVERRIDE	0x0200

#define VM_SINGLE_TAGGED(mask)		\
	((mask & VM_CVLAN_TAGGED ) | (mask & VM_SVLAN_TAGGED))
#define VM_DOUBLE_TAGGED(mask)		\
	(mask & (VM_CVLAN_TAGGED | VM_SVLAN_TAGGED))



			quint16	ctpid;
			quint16	cvlanPrio	:  3;
			quint16	cvlanCfi	:  1;
			quint16	cvlanId		: 13;
			quint16	stpid;
			quint16	svlanPrio	:  3;
			quint16	svlanCfi	:  1;
			quint16	svlanId		: 13;
		} eth;
	} l2;

	struct {
		// IP
		struct {
			quint8		ipMask;
#define IM_OVERRIDE_VERSION		0x01
#define IM_OVERRIDE_HDRLEN		0x02
#define IM_OVERRIDE_TOTLEN		0x04
#define IM_OVERRIDE_CKSUM		0x08
#define STREAM_DEF_IP_MASK			0x00

			quint8		ver		: 4;
#define STREAM_DEF_L3_IP_VER		0x4

			quint8		hdrLen	: 4;
#define STREAM_DEF_L3_IP_HDR_LEN	0x5

			quint8		tos;
#define STREAM_DEF_L3_IP_TOS		0x00

			quint16		totLen;
#define STREAM_DEF_L3_IP_TOT_LEN	0x00

			quint16		id;
#define STREAM_DEF_L3_IP_ID			0x1234

			quint16		flags	:  3;
#define IP_FLAG_UNUSED	0x1
#define IP_FLAG_DF		0x2
#define IP_FLAG_MF		0x4
#define STREAM_DEF_L3_IP_FLAGS		0x00

			quint16		fragOfs	: 13;
#define STREAM_DEF_L3_IP_FRAG_OFS	0x0000

			quint8		ttl;
#define STREAM_DEF_L3_IP_TTL		0x7F

			quint8		proto;
#define STREAM_DEF_L3_IP_PROTO		0x00

			quint16		cksum;
#define STREAM_DEF_L3_IP_CKSUM		0x0000

			// Source IP
			quint32		srcIp;
#define STREAM_DEF_L3_IP_SRC_IP		0x02020202

			IpAddrMode	srcIpMode;
#define STREAM_DEF_L3_IP_SRC_IP_MODE	e_im_fixed

			quint16		srcIpCount;
#define STREAM_DEF_L3_IP_SRC_IP_COUNT	16

			quint32		srcIpMask;
#define STREAM_DEF_L3_IP_SRC_IP_MASK	0xFFFFFFFF
			
			// Destination IP
			quint32		dstIp;
#define STREAM_DEF_L3_IP_DST_IP			0x01010101

			IpAddrMode	dstIpMode;
#define STREAM_DEF_L3_IP_DST_IP_MODE	e_im_fixed

			quint16		dstIpCount;
#define STREAM_DEF_L3_IP_DST_IP_COUNT	16

			quint32		dstIpMask;
#define STREAM_DEF_L3_IP_DST_IP_MASK	0xFFFFFFFF

			// TODO: Options
		} ip;

		// TODO: ARP
		struct {
		} arp;
	} l3;

	// L4
	struct {
		// TCP
		struct {
			quint32		tcpMask;
#define TM_OVERRIDE_HDRLEN		0x1
#define TM_OVERRIDE_CKSUM		0x2
#define STREAM_DEF_L4_TCP_TCP_MASK		0x00;

			quint16		srcPort;
#define STREAM_DEF_L4_TCP_SRC_PORT		8902;

			quint16		dstPort;
#define STREAM_DEF_L4_TCP_DST_PORT		80

			quint32		seqNum;
#define STREAM_DEF_L4_TCP_SEQ_NUM		129018

			quint32		ackNum;
#define STREAM_DEF_L4_TCP_ACK_NUM		98223

			quint8		hdrLen	: 4;
#define STREAM_DEF_L4_TCP_HDR_LEN		0x5

			quint8		rsvd	: 4;
#define STREAM_DEF_L4_TCP_RSVD			0x0

			quint8		flags;
#define TCP_FLAG_URG	0x01
#define TCP_FLAG_ACK	0x02
#define TCP_FLAG_PSH	0x04
#define TCP_FLAG_RST	0x08
#define TCP_FLAG_SYN	0x10
#define TCP_FLAG_FIN	0x20
#define STREAM_DEF_L4_TCP_FLAGS			0x00


			quint16		window;
#define STREAM_DEF_L4_TCP_WINDOW		1024

			quint16		cksum;
#define STREAM_DEF_L4_TCP_CKSUM			0x0000

			quint16		urgPtr;
#define STREAM_DEF_L4_TCP_URG_PTR		0x0000
		} tcp;
		
		// UDP
		struct {
			quint32		udpMask;
#define UM_OVERRIDE_TOTLEN		0x01
#define UM_OVERRIDE_CKSUM		0x02
#define STREAM_DEF_L4_UDP_UDP_MASK		0x00

			quint16		srcPort;
#define STREAM_DEF_L4_UDP_SRC_PORT		8902

			quint16		dstPort;
#define STREAM_DEF_L4_UDP_DST_PORT		80

			quint16		totLen;
#define STREAM_DEF_L4_UDP_TOT_LEN		0x0000

			quint16		cksum;
#define STREAM_DEF_L4_UDP_CKSUM			0x0000
		} udp;

		// TODO: ICMP
		struct {
		} icmp;

		// TODO: IGMP
		struct {
		} igmp;
	} l4;

	QString		mName;
	bool		mIsEnabled;

// -------------------------------------------------------
// Methods
// -------------------------------------------------------
public:
	Stream();
	int enable(bool flag);
	const QString& name() const { return mName; }
	bool isEnabled() const { return mIsEnabled; }

	void setName(QString name) { mName = name; }
	void setEnabled(bool isEnabled) { mIsEnabled = isEnabled; }

private:
	void InitDefaultMeta();
	void InitDefaultProto();
	void InitDefaultL2();
	void InitDefaultL3();
	void InitDefaultL3Ip();
	void InitDefaultL4();
	void InitDefaultL4Tcp();
	void InitDefaultL4Udp();
};

#endif
