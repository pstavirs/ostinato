#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>
#include <QString>
#include "../common/protocol.pb.h"

class StreamConfigDialog;
class StreamModel;
class PacketModel;

// Convenience Defines FIXME
#define IP_PROTO_ICMP	0x01
#define IP_PROTO_IGMP	0x02
#define IP_PROTO_TCP	0x06
#define IP_PROTO_UDP	0x11

#if 0
	// Protocols
	struct {
		FrameType	ft;



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




		} eth;
	} l2;
#endif

class AbstractProtocol
{
	// TODO
};

class MacProtocol : public AbstractProtocol 
{
private:
	OstProto::Mac	d;

public:
	enum MacAddrMode {
		MacAddrFixed,
		MacAddrInc,
		MacAddrDec
	};

	// Dst Mac
	quint64 dstMac()
		{ return d.dst_mac(); }

	bool	setDstMac(quint64 dstMac)
		{ d.set_dst_mac(dstMac); return true; }

	MacAddrMode	dstMacMode()
		{ return (MacAddrMode) d.dst_mac_mode(); }
	bool		setDstMacMode(MacAddrMode dstMacMode)
		{ d.set_dst_mac_mode((OstProto::Mac::MacAddrMode)dstMacMode); return true; }

	quint16	dstMacCount()
		{ return d.dst_mac_count(); }
	bool	setDstMacCount(quint16 dstMacCount)
		{ d.set_dst_mac_count(dstMacCount); return true; }

	quint16	dstMacStep()
		{ return d.dst_mac_step(); }
	bool	setDstMacStep(quint16 dstMacStep)
		{ d.set_dst_mac_step(dstMacStep); return true; }

	// Src Mac
	quint64 srcMac()
		{ return d.src_mac(); }

	bool	setSrcMac(quint64 srcMac)
		{ d.set_src_mac(srcMac); return true; }

	MacAddrMode	srcMacMode()
		{ return (MacAddrMode) d.src_mac_mode(); }
	bool		setSrcMacMode(MacAddrMode srcMacMode)
		{ d.set_src_mac_mode((OstProto::Mac::MacAddrMode)srcMacMode); return true; }

	quint16	srcMacCount()
		{ return d.src_mac_count(); }
	bool	setSrcMacCount(quint16 srcMacCount)
		{ d.set_src_mac_count(srcMacCount); return true; }

	quint16	srcMacStep()
		{ return d.src_mac_step(); }
	bool	setSrcMacStep(quint16 srcMacStep)
		{ d.set_src_mac_step(srcMacStep); return true; }
};

class LlcProtocol : public AbstractProtocol
{
private:
	OstProto::Llc	d;

public:
	quint8	dsap()
		{ return d.dsap(); }
	bool	setDsap(quint8 dsap)
		{ d.set_dsap(dsap); return true; }

	quint8	ssap()
		{ return d.ssap(); }
	bool	setSsap(quint8 ssap)
		{ d.set_ssap(ssap); return true; }

	quint8	ctl()
		{ return d.ctl(); }
	bool	setCtl(quint8 ctl)
		{ d.set_ctl(ctl); return true; }

};

class SnapProtocol : public AbstractProtocol
{
private:
	OstProto::Snap	d;

public:
	quint32	oui()
		{ return d.oui(); }
	bool	setOui(quint32 oui)
		{ d.set_oui(oui); return true; }

	quint16	type()
		{ return d.type(); }
	bool	setType(quint16	type)
		{ d.set_type(type); return true; }
};

class Eth2Protocol : public AbstractProtocol
{
private:
	OstProto::Eth2	d;

public:
	quint16	type()
		{ return d.type(); }
	bool	setType(quint16	type)
		{ d.set_type(type); return true; }
};

class VlanProtocol : public AbstractProtocol
{
// TODO
#if 0
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
#endif
};

// IP
class IpProtocol : public AbstractProtocol
{
private:
	OstProto::Ip d;

public:

	enum IpAddrMode {
		IpAddrFixed,
		IpAddrIncHost,
		IpAddrDecHost,
		IpAddrRandomHost
	};

	enum IpFlag {
		IpOverrideVersion = 0x01,
		IpOverrideHdrLen = 0x02,
		IpOverrideTotLen = 0x03,
		IpOverrideCksum = 0x04
	};
	Q_DECLARE_FLAGS(IpFlags, IpFlag);

	IpFlags ipFlags()
		{
			IpFlags f;

			if (d.is_override_ver()) f|= IpOverrideVersion;
			if (d.is_override_hdrlen()) f|= IpOverrideHdrLen;
			if (d.is_override_totlen()) f|= IpOverrideTotLen;
			if (d.is_override_cksum()) f|= IpOverrideCksum;

			return f;
		}

	bool	setIpFlags(IpFlags ipFlags)
		{
			if (ipFlags.testFlag(IpOverrideVersion))
				d.set_is_override_ver(true);
			else
				d.set_is_override_ver(false);

			if (ipFlags.testFlag(IpOverrideHdrLen))
				d.set_is_override_hdrlen(true);
			else
				d.set_is_override_hdrlen(false);

			if (ipFlags.testFlag(IpOverrideTotLen))
				d.set_is_override_totlen(true);
			else
				d.set_is_override_totlen(false);

			if (ipFlags.testFlag(IpOverrideCksum))
				d.set_is_override_cksum(true);
			else
				d.set_is_override_cksum(false);

			return true;
		}

	quint8	ver()
		{ return (d.ver_hdrlen() >> 4); }
	bool	setVer(quint8 ver)
		{ d.set_ver_hdrlen((d.ver_hdrlen() & 0x0F) | (ver << 4)); return true; }

	quint8	hdrLen()
		{ return (d.ver_hdrlen() & 0xF); }
	bool	setHdrLen(quint8 hdrLen)
		{ d.set_ver_hdrlen((d.ver_hdrlen() & 0xF0) | hdrLen); return true; }

	quint8	tos()
		{ return d.tos(); }
	bool	setTos(quint8 tos)
		{ d.set_tos(tos); return true; }

	quint16	totLen()
		{ return d.tot_len(); }
	bool	setTotLen(quint16 totLen)
		{ d.set_tot_len(totLen); return true; }

	quint16	id()
		{ return d.id(); }
	bool	setId(quint16 id)
		{ d.set_id(id); return true; }

	quint16	flags()
		{ return d.flags(); }
	bool	setFlags(quint16 flags)
		{ d.set_flags(flags); return true; }
#define IP_FLAG_UNUSED	0x1
#define IP_FLAG_DF		0x2
#define IP_FLAG_MF		0x4

	quint16	fragOfs()
		{ return d.frag_ofs(); }
	bool setFragOfs(quint16	fragOfs)
		{ d.set_frag_ofs(fragOfs); return true; }

	quint8	ttl()
		{ return d.ttl(); }
	bool	setTtl(quint8 ttl)
		{ d.set_ttl(ttl); return true; }

	quint8	proto()
		{ return d.proto(); }
	bool	setProto(quint8	proto)
		{ d.set_proto(proto); return true; }

	quint16	cksum()
		{ return d.cksum(); }
	bool	setCksum(quint16 cksum)
		{ d.set_cksum(cksum); return true; }

	// Source IP
	quint32	srcIp()
		{ return d.src_ip(); }
	bool	setSrcIp(quint32 srcIp)
		{ d.set_src_ip(srcIp); return true; }

	IpAddrMode	srcIpMode()
		{ return (IpAddrMode) d.src_ip_mode(); }
	bool		setSrcIpMode(IpAddrMode srcIpMode)
		{ d.set_src_ip_mode((OstProto::Ip::IpAddrMode)srcIpMode); return true; }

	quint16	srcIpCount()
		{ return d.src_ip_count(); }
	bool	setSrcIpCount(quint16 srcIpCount)
		{ d.set_src_ip_count(srcIpCount); return true; }

	quint32	srcIpMask()
		{ return d.src_ip_mask(); }
	bool	setSrcIpMask(quint32 srcIpMask)
		{ d.set_src_ip_mask(srcIpMask); return true; }

	// Destination IP
	quint32	dstIp()
		{ return d.dst_ip(); }
	bool	setDstIp(quint32 dstIp)
		{ d.set_dst_ip(dstIp); return true; }

	IpAddrMode	dstIpMode()
		{ return (IpAddrMode) d.dst_ip_mode(); }
	bool		setDstIpMode(IpAddrMode dstIpMode)
		{ d.set_dst_ip_mode((OstProto::Ip::IpAddrMode)dstIpMode); return true; }

	quint16	dstIpCount()
		{ return d.dst_ip_count(); }
	bool	setDstIpCount(quint16 dstIpCount)
		{ d.set_dst_ip_count(dstIpCount); return true; }

	quint32	dstIpMask()
		{ return d.dst_ip_mask(); }
	bool	setDstIpMask(quint32 dstIpMask)
		{ d.set_dst_ip_mask(dstIpMask); return true; }
	
	// TODO: Options
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IpProtocol::IpFlags)

class ArpProtocol: public AbstractProtocol
{
	// TODO: ARP
};

// TCP
class TcpProtocol : public AbstractProtocol
{
private:
	OstProto::Tcp d;

public:
	enum TcpFlag
	{
		TcpOverrideHdrLen = 0x01,
		TcpOverrideCksum = 0x02
	};
	Q_DECLARE_FLAGS(TcpFlags, TcpFlag);

	TcpFlags tcpFlags()
		{
			TcpFlags f;

			if (d.is_override_hdrlen()) f|= TcpOverrideHdrLen;
			if (d.is_override_cksum()) f|= TcpOverrideCksum;

			return f;
		}

	bool	setTcpFlags(TcpFlags tcpFlags)
		{
			if (tcpFlags.testFlag(TcpOverrideHdrLen))
				d.set_is_override_hdrlen(true);
			else
				d.set_is_override_hdrlen(false);

			if (tcpFlags.testFlag(TcpOverrideCksum))
				d.set_is_override_cksum(true);
			else
				d.set_is_override_cksum(false);

			return true;
		}

	quint16	srcPort()
		{ return d.src_port(); }
	bool	setSrcPort(quint16 srcPort)
		{ d.set_src_port(srcPort); return true; }

	quint16	dstPort()
		{ return d.dst_port(); }
	bool	setdstPort(quint16 dstPort)
		{ d.set_dst_port(dstPort); return true; }

	quint32	seqNum()
		{ return d.seq_num(); }
	bool 	setSeqNum(quint32 seqNum)
		{ d.set_seq_num(seqNum); return true;}

	quint32	ackNum()
		{ return d.ack_num(); }
	bool 	setAckNum(quint32 ackNum)
		{ d.set_ack_num(ackNum); return true;}

	quint8	hdrLen()
		{ return (d.hdrlen_rsvd() >> 4); }
	bool	setHdrLen(quint8 hdrLen)
		{ d.set_hdrlen_rsvd((d.hdrlen_rsvd() & 0x0F) | (hdrLen << 4)); return true; }

	quint8	rsvd()
		{ return (d.hdrlen_rsvd() & 0xF); }
	bool	setRsvd(quint8 rsvd)
		{ d.set_hdrlen_rsvd((d.hdrlen_rsvd() & 0xF0) | rsvd); return true; }


	// TODO: convert to enum maybe?
	quint8	flags()
		{ return d.flags(); }
	bool setFlags(quint8 flags)
		{ d.set_flags(flags); return true; }
#define TCP_FLAG_URG	0x01
#define TCP_FLAG_ACK	0x02
#define TCP_FLAG_PSH	0x04
#define TCP_FLAG_RST	0x08
#define TCP_FLAG_SYN	0x10
#define TCP_FLAG_FIN	0x20

	quint16	window()
		{ return d.window(); }
	bool	setWindow(quint16 window)
		{ d.set_window(window); return true; }

	quint16	cksum()
		{ return d.cksum(); }
	bool	setCksum(quint16 cksum)
		{ d.set_cksum(cksum); return true; }

	quint16	urg_ptr()
		{ return d.urg_ptr(); }
	bool	seturg_ptr(quint16 urg_ptr)
		{ d.set_urg_ptr(urg_ptr); return true; }

};
Q_DECLARE_OPERATORS_FOR_FLAGS(TcpProtocol::TcpFlags)

		
// UDP
class UdpProtocol : public AbstractProtocol
{
private:
	OstProto::Udp d;

public:
	enum UdpFlag
	{
		UdpOverrideTotLen = 0x01,
		UdpOverrideCksum = 0x02
	};
	Q_DECLARE_FLAGS(UdpFlags, UdpFlag);

	UdpFlags udpFlags()
		{
			UdpFlags f;

			if (d.is_override_totlen()) f|= UdpOverrideTotLen;
			if (d.is_override_cksum()) f|= UdpOverrideCksum;

			return f;
		}

	bool	setUdpFlags(UdpFlags udpFlags)
		{
			if (udpFlags.testFlag(UdpOverrideTotLen))
				d.set_is_override_totlen(true);
			else
				d.set_is_override_totlen(false);

			if (udpFlags.testFlag(UdpOverrideCksum))
				d.set_is_override_cksum(true);
			else
				d.set_is_override_cksum(false);

			return true;
		}

	quint16	srcPort()
		{ return d.src_port(); }
	bool	setSrcPort(quint16 srcPort)
		{ d.set_src_port(srcPort); return true; }

	quint16	dstPort()
		{ return d.dst_port(); }
	bool	setdstPort(quint16 dstPort)
		{ d.set_dst_port(dstPort); return true; }

	quint16	totLen()
		{ return d.totlen(); }
	bool	setTotLen(quint16 totLen)
		{ d.set_totlen(totLen); return true; }

	quint16	cksum()
		{ return d.cksum(); }
	bool	setCksum(quint16 cksum)
		{ d.set_cksum(cksum); return true; }

};

class IcmpProtocol {
// TODO: ICMP
};

class IgmpProtocol {
// TODO: IGMP
};


class Stream {

	static quint32			mAllocId;

	quint32					mId;
	OstProto::StreamCore	*mCore;

	MacProtocol	*mMac;
	IpProtocol	*mIp;

#if 0
	friend class StreamConfigDialog;
	friend class StreamModel;
	friend class PacketModel;
#endif

public:
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

	// -------------------------------------------------------
	// Methods
	// -------------------------------------------------------
	Stream();

	quint32	id()
		{ return mId;}

	quint32	ordinal()
		{ return mCore->ordinal();}
	bool setOrderdinal(quint32	ordinal)
		{ mCore->set_ordinal(ordinal); return true; }

	bool isEnabled() const
		{ return mCore->is_enabled(); }
	bool setIsEnabled(bool flag)
		{ mCore->set_is_enabled(flag); return true; }

	const QString name() const 
		{ return QString().fromStdString(mCore->name()); }
	bool setName(QString name) 
		{ mCore->set_name(name.toStdString()); return true; }

	FrameType	frameType()
		{ return (FrameType) mCore->ft(); }
	bool		setFrameType(FrameType	frameType)
		{ mCore->set_ft((OstProto::StreamCore::FrameType) frameType); return true; }

	// Data Pattern
	DataPatternMode	patternMode()
		{ return (DataPatternMode) mCore->pattern_mode(); }
	bool setPatternMode(DataPatternMode patternMode)
		{ mCore->set_pattern_mode(
			(OstProto::StreamCore::DataPatternMode) patternMode); return true; }

	quint32	 pattern()
		{ return mCore->pattern(); }
	bool setPattern(quint32 pattern)
		{ mCore->set_pattern(pattern); return true; }

	// Frame Length (includes CRC)
	FrameLengthMode	lenMode()
		{ return (FrameLengthMode) mCore->len_mode(); }
	bool setLenMode(FrameLengthMode	lenMode)
		{ mCore->set_len_mode(
			(OstProto::StreamCore::FrameLengthMode) lenMode); return true; }

	quint16	frameLen()
		{ return mCore->frame_len(); }
	bool setFrameLen(quint16 frameLen)
		{ mCore->set_frame_len(frameLen);  return true; }

	quint16	frameLenMin()
		{ return mCore->frame_len_min(); }
	bool setFrameLenMin(quint16 frameLenMin)
		{ mCore->set_frame_len_min(frameLenMin);  return true; }

	quint16	frameLenMax()
		{ return mCore->frame_len_max(); }
	bool setFrameLenMax(quint16 frameLenMax)
		{ mCore->set_frame_len_max(frameLenMax);  return true; }

// TODO
#if 0
	quint16			dataStartOfs;
#endif

	MacProtocol* mac() { return mMac; }
	IpProtocol* ip() { return mIp; }

private:
#if 0
	void InitDefaultMeta();
	void InitDefaultProto();
	void InitDefaultL2();
	void InitDefaultL3();
	void InitDefaultL3Ip();
	void InitDefaultL4();
	void InitDefaultL4Tcp();
	void InitDefaultL4Udp();
#endif
};

#endif
