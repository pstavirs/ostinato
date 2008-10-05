#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>

#include <QString>
#include <QList>
#include "../common/protocol.pb.h"

class StreamConfigDialog;
class StreamModel;
class PacketModel;

// Convenience Defines FIXME
#define IP_PROTO_ICMP	0x01
#define IP_PROTO_IGMP	0x02
#define IP_PROTO_TCP	0x06
#define IP_PROTO_UDP	0x11


class Stream;

class AbstractProtocol
{
	// TODO(LOW)
public:
	/*!
	  Subclasses should return reference to their protocol specific 
	  ::google::protobuf::Message
	*/
	virtual ::google::protobuf::Message& data() = 0;
	/*! Subclasses can directly use this method. No need for overload */
	void getConfig(::google::protobuf::Message *msg)
		{ msg->CopyFrom(data()); }

	virtual QString protocolName()
		{ return QString("AbstractProtocol"); }
	virtual QString protocolShortName()
		{ return QString("AbsProto"); }
	virtual int	numFields()
		{ return 1; }
	QByteArray protocolRawValue()
	{
		QByteArray ba;
#ifndef SRIVATSP
#else
		for (int i=0; i < numFields(); i++) 
			ba.append(fieldRawValue(i));
#endif
		return ba;
	}
	virtual QString fieldName(int index)
		{ return QString("AbstractField"); }
	virtual QString fieldTextValue(int index)
		{ return QString("AbstractFieldValue"); }
	virtual QByteArray fieldRawValue(int index)
		{ return QByteArray(4, '\0'); }
};

class UnknownProtocol: public AbstractProtocol
{
	OstProto::Ack	d;		// FIXME(HI): replace 'Ack' with something else

public:
	virtual ~UnknownProtocol() {}

	virtual ::google::protobuf::Message& data() { return d; }

	virtual QString protocolName()
		{ return QString("UnknownProtocol"); }
	QString protocolShortName()
		{ return QString("???Proto"); }
	int	numFields()
		{ return 1; }
	QString fieldName(int index)
		{ return QString("UnknownField"); }
	QString fieldTextValue(int index)
		{ return QString("UnknownFieldValue"); }
	QByteArray fieldRawValue(int index)
		{ return QByteArray(); }
};

class PayloadProtocol: public AbstractProtocol
{
	Stream			*parentStream;
	OstProto::Ack	d;		// FIXME(HI): move payload related vars from
							// stream into here

public:
	PayloadProtocol (Stream *stream = NULL) { parentStream = stream; }
	virtual ~PayloadProtocol() {}

	virtual ::google::protobuf::Message& data() { return d; }

	virtual QString protocolName()
		{ return QString("Payload Data"); }
	QString protocolShortName()
		{ return QString("DATA"); }
	int	numFields()
		{ return 1; }
	QString fieldName(int index)
		{ return QString("Data"); }
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
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
	virtual ~MacProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 

	bool update(OstProto::Mac	mac) { d.MergeFrom(mac); return true; }

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


	virtual QString protocolName()
		{ return QString("Media Access Control"); } 
	QString protocolShortName()
		{ return QString("MAC"); } 
	int	numFields()
		{ return 2; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

class LlcProtocol : public AbstractProtocol
{
private:
	OstProto::Llc	d;

public:
	virtual ~LlcProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 

	bool update(OstProto::Llc	llc) { d.MergeFrom(llc); return true; }

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


	virtual QString protocolName()
		{ return QString("802.3 Logical Link Control"); } 
	QString protocolShortName()
		{ return QString("LLC"); } 
	int	numFields()
		{ return 3; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

class SnapProtocol : public AbstractProtocol
{
private:
	OstProto::Snap	d;

public:
	virtual ~SnapProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Snap	snap) { d.MergeFrom(snap); return true; }

	quint32	oui()
		{ return d.oui(); }
	bool	setOui(quint32 oui)
		{ d.set_oui(oui); return true; }

// "Type" field: use from eth2 
#if 0
	quint16	type()
		{ return d.type(); }
	bool	setType(quint16	type)
		{ d.set_type(type); return true; }
#endif
	virtual QString protocolName()
		{ return QString("SubNetwork Access Protocol"); } 
	QString protocolShortName()
		{ return QString("SNAP"); } 
	int	numFields()
		{ return 1; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};


class Eth2Protocol : public AbstractProtocol
{
private:
	OstProto::Eth2	d;

public:
	virtual ~Eth2Protocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Eth2	eth2) { d.MergeFrom(eth2); return true; }

	quint16	type()
		{ return d.type(); }
	bool	setType(quint16	type)
		{ d.set_type(type); return true; }


	virtual QString protocolName()
		{ return QString("Protocol Type"); } 
	QString protocolShortName()
		{ return QString("TYPE"); } 
	int	numFields()
		{ return 1; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};


class Dot3Protocol : public AbstractProtocol
{
private:
	Stream			*parentStream;
	OstProto::Ack	d;		// FIXME(HI): replace 'ack' with somehting else

public:
	Dot3Protocol(Stream *stream = NULL) 
		{ parentStream = stream; qDebug("parentStream = %p", stream); }
	virtual ~Dot3Protocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 

#if 0 // FIXME(HI): remove?
	quint16	length()
		{ return d.length(); }
	bool	setLength(quint16	length)
		{ return true; }
#endif

	virtual QString protocolName()
		{ return QString("802.3 Length"); } 
	QString protocolShortName()
		{ return QString("LEN"); } 
	int	numFields()
		{ return 1; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

class VlanProtocol : public AbstractProtocol
{
	OstProto::Vlan	d;
public:
	virtual ~VlanProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Vlan	vlan) { d.MergeFrom(vlan); return true; }

	enum VlanFlag {
		VlanCvlanTagged = 0x01,
		VlanCtpidOverride = 0x02,
		VlanSvlanTagged = 0x04,
		VlanStpidOverride = 0x08,
	};
	Q_DECLARE_FLAGS(VlanFlags, VlanFlag);

	VlanFlags vlanFlags()
		{
			VlanFlags f;

			if (d.is_cvlan_tagged()) f|= VlanCvlanTagged;
			if (d.is_ctpid_override()) f|= VlanCtpidOverride;
			if (d.is_svlan_tagged()) f|= VlanSvlanTagged;
			if (d.is_stpid_override()) f|= VlanStpidOverride;

			return f;
		}

	bool	setVlanFlags(VlanFlags vlanFlags)
		{
			d.set_is_cvlan_tagged(vlanFlags.testFlag(VlanCvlanTagged));
			d.set_is_ctpid_override(vlanFlags.testFlag(VlanCtpidOverride));
			d.set_is_svlan_tagged(vlanFlags.testFlag(VlanSvlanTagged));
			d.set_is_stpid_override(vlanFlags.testFlag(VlanStpidOverride));

			return true;
		}

	bool	isUntagged()
	{
		if (!d.is_cvlan_tagged() && !d.is_svlan_tagged())
			return true;
		else
			return false;
	}

	bool	isSingleTagged()
	{
		if (( d.is_cvlan_tagged() && !d.is_svlan_tagged()) ||
			(!d.is_cvlan_tagged() &&  d.is_svlan_tagged()) )
			return true;
		else
			return false;
	}

	bool	isDoubleTagged()
	{
		if (d.is_cvlan_tagged() && d.is_svlan_tagged())
			return true;
		else
			return false;
	}

	// CVLAN
	quint16	ctpid()
		{ return d.ctpid(); }
	bool setCtpid(quint16	ctpid)
		{ d.set_ctpid(ctpid); return true; }

	quint8	cvlanPrio()
		{ return (d.cvlan_tag() >> 13); }
	bool setCvlanPrio(quint8 cvlanPrio)
		{ d.set_cvlan_tag((d.cvlan_tag() & 0x1FFF) | ((cvlanPrio & 0x3) << 13));
			return true; }

	quint8	cvlanCfi()
		{ return ((d.cvlan_tag() & 0x1000) >> 12); }
	bool setCvlanCfi(quint8 cvlanCfi)
		{ d.set_cvlan_tag((d.cvlan_tag() & 0xEFFF) | ((cvlanCfi & 0x01) << 12));
			return true; }

	quint16	cvlanId()
		{ return (d.cvlan_tag() & 0x0FFF); }
	bool setCvlanId(quint16 cvlanId)
		{ d.set_cvlan_tag((d.cvlan_tag() & 0xF000) | ((cvlanId & 0x0FFF)));
			return true; }

	// SVLAN
	quint16	stpid()
		{ return d.stpid(); }
	bool setStpid(quint16	stpid)
		{ d.set_stpid(stpid); return true; }
	quint8	svlanPrio()
		{ return (d.svlan_tag() >> 13); }
	bool setSvlanPrio(quint8 svlanPrio)
		{ d.set_svlan_tag((d.svlan_tag() & 0x1FFF) | ((svlanPrio & 0x3) << 13));
			return true; }

	quint8	svlanCfi()
		{ return ((d.svlan_tag() & 0x1000) >> 12); }
	bool setSvlanCfi(quint8 svlanCfi)
		{ d.set_svlan_tag((d.svlan_tag() & 0xEFFF) | ((svlanCfi & 0x01) << 12));
			return true; }

	quint16	svlanId()
		{ return (d.svlan_tag() & 0x0FFF); }
	bool setSvlanId(quint16 svlanId)
		{ d.set_svlan_tag((d.svlan_tag() & 0xF000) | ((svlanId & 0x0FFF)));
			return true; }

	virtual QString protocolName()
		{ return QString("Virtual Local Access Network"); } 
	QString protocolShortName()
		{ return QString("VLAN"); } 
	int	numFields();
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

// IP
class IpProtocol : public AbstractProtocol
{
private:
	OstProto::Ip d;

public:
	virtual ~IpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 

	bool update(OstProto::Ip ip) { d.MergeFrom(ip); return true; }

	enum IpAddrMode {
		IpAddrFixed,
		IpAddrIncHost,
		IpAddrDecHost,
		IpAddrRandomHost
	};

	enum IpFlag {
		IpOverrideVersion = 0x01,
		IpOverrideHdrLen = 0x02,
		IpOverrideTotLen = 0x04,
		IpOverrideCksum = 0x08
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
	
	// TODO(LOW): Options


	virtual QString protocolName()
		{ return QString("Internet Protocol version 4"); } 
	QString protocolShortName()
		{ return QString("IPv4"); } 
	int	numFields()
		{ return 12; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IpProtocol::IpFlags)

class ArpProtocol: public AbstractProtocol
{
	// TODO(LOW): ARP
	OstProto::Arp	d;

public:
	virtual ~ArpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Arp	arp) { d.MergeFrom(arp); return true; }
};

// TCP
class TcpProtocol : public AbstractProtocol
{
private:
	OstProto::Tcp d;

public:
	virtual ~TcpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 

	bool update(OstProto::Tcp	tcp) { d.MergeFrom(tcp); return true; }

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
	bool	setDstPort(quint16 dstPort)
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


	// TODO(MED): convert to enum maybe?
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

	quint16	urgPtr()
		{ return d.urg_ptr(); }
	bool	setUrgPtr(quint16 urg_ptr)
		{ d.set_urg_ptr(urg_ptr); return true; }


	virtual QString protocolName()
		{ return QString("Transmission Control Protocol"); } 
	QString protocolShortName()
		{ return QString("TCP"); } 
	int	numFields()
		{ return 10; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(TcpProtocol::TcpFlags)

		
// UDP
class UdpProtocol : public AbstractProtocol
{
private:
	OstProto::Udp d;

public:
	virtual ~UdpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	
	bool update(OstProto::Udp	udp) { d.MergeFrom(udp); return true; }

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
	bool	setDstPort(quint16 dstPort)
		{ d.set_dst_port(dstPort); return true; }

	quint16	totLen()
		{ return d.totlen(); }
	bool	setTotLen(quint16 totLen)
		{ d.set_totlen(totLen); return true; }

	quint16	cksum()
		{ return d.cksum(); }
	bool	setCksum(quint16 cksum)
		{ d.set_cksum(cksum); return true; }


	virtual QString protocolName()
		{ return QString("User Datagram Protocol"); } 
	QString protocolShortName()
		{ return QString("UDP"); } 
	int	numFields()
		{ return 4; }
	QString fieldName(int index);
	QString fieldTextValue(int index);
	QByteArray fieldRawValue(int index);
};

class IcmpProtocol : public AbstractProtocol
{
// TODO(LOW): ICMP
	OstProto::Icmp	d;

public:
	virtual ~IcmpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Icmp	icmp) { d.MergeFrom(icmp); return true; }
};

class IgmpProtocol  : public AbstractProtocol
{
// TODO(LOW): IGMP
	OstProto::Igmp	d;

public:
	virtual ~IgmpProtocol() {}
	virtual ::google::protobuf::Message& data() {return d;} 
	bool update(OstProto::Igmp	igmp) { d.MergeFrom(igmp); return true; }
};


class Stream {

	quint32					mId;
	OstProto::StreamCore	*mCore;
	OstProto::StreamControl	*mControl;

	UnknownProtocol	*mUnknown;
	PayloadProtocol	*mPayload;
	MacProtocol		*mMac;

	LlcProtocol		*mLlc;
	SnapProtocol	*mSnap;
	Eth2Protocol	*mEth2;
	Dot3Protocol	*mDot3;

	VlanProtocol	*mVlan;

	IpProtocol		*mIp;
	ArpProtocol		*mArp;

	TcpProtocol		*mTcp;
	UdpProtocol		*mUdp;
	IcmpProtocol	*mIcmp;
	IgmpProtocol	*mIgmp;

public:
	//PayloadProtocol* Payload() { return mPayload; }
	MacProtocol* mac() { return mMac; }

	void* core() { return mCore; } // FIXME(HI): Debug ONLY

	LlcProtocol* llc() { return mLlc; }
	SnapProtocol* snap() { return mSnap; }
	Eth2Protocol* eth2() { return mEth2; }
	Dot3Protocol* dot3() { return mDot3; }
	VlanProtocol* vlan() { return mVlan; }

	IpProtocol* ip() { return mIp; }
	ArpProtocol* arp() { return mArp; }

	TcpProtocol* tcp() { return mTcp; }
	UdpProtocol* udp() { return mUdp; }
	IcmpProtocol* icmp() { return mIcmp; }
	IgmpProtocol* igmp() { return mIgmp; }


public:
	enum FrameType {
		e_ft_none,
		e_ft_eth_2,
		e_ft_802_3_raw,
		e_ft_802_3_llc,
		e_ft_snap
	};

	enum DataPatternMode {
		e_dp_fixed_word,
		e_dp_inc_byte,
		e_dp_dec_byte,
		e_dp_random
	};

	enum FrameLengthMode {
		e_fl_fixed,
		e_fl_inc,
		e_fl_dec,
		e_fl_random
	};

	enum L3Proto {
		e_l3_none,
		e_l3_ip,
		e_l3_arp,
	};

	enum L4Proto {
		e_l4_none,
		e_l4_tcp,
		e_l4_udp,
		e_l4_icmp,
		e_l4_igmp,
	};

	enum SendUnit {
		e_su_packets,
		e_su_bursts
	};

	enum SendMode {
		e_sm_fixed,
		e_sm_continuous
	};

	enum NextWhat {
		e_nw_stop,
		e_nw_goto_next,
		e_nw_goto_id
	};

	// -------------------------------------------------------
	// Methods
	// -------------------------------------------------------
	Stream();

	bool operator < (const Stream &s) const
		{ return(mCore->ordinal() < s.mCore->ordinal()); }


	void getConfig(uint portId, OstProto::Stream *s);
	bool update(OstProto::Stream *stream);

	quint32	id()
		{ return mId;}
	bool setId(quint32 id)
		{ mId = id; return true;}

#if 0 // FIXME(HI): needed?
	quint32	portId()
		{ return mCore->port_id();}
	bool setPortId(quint32 id)
		{ mCore->set_port_id(id); return true;}
#endif

	quint32	ordinal()
		{ return mCore->ordinal();}
	bool setOrdinal(quint32	ordinal)
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

// TODO(HI) : ?????
#if 0
	quint16			dataStartOfs;
#endif

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

	L3Proto l3Proto()
		{ return (L3Proto) mCore->l3_proto(); }
	bool setL3Proto(L3Proto l3Proto)
		{ mCore->set_l3_proto((OstProto::StreamCore::L3Proto) l3Proto); 
			return true; }

	L4Proto l4Proto()
		{ return (L4Proto) mCore->l4_proto(); }
	bool setL4Proto(L4Proto l4Proto)
		{ mCore->set_l4_proto((OstProto::StreamCore::L4Proto) l4Proto); 
			return true; }

	SendUnit sendUnit()
		{ return (SendUnit) mControl->unit(); }
	bool setSendUnit(SendUnit sendUnit)
		{ mControl->set_unit(
			(OstProto::StreamControl::SendUnit) sendUnit); return true; }

	SendMode sendMode()
		{ return (SendMode) mControl->mode(); }
	bool setSendMode(SendMode sendMode)
		{ mControl->set_mode(
			(OstProto::StreamControl::SendMode) sendMode); return true; }

	NextWhat nextWhat()
		{ return (NextWhat) mControl->next(); }
	bool setNextWhat(NextWhat nextWhat)
		{ mControl->set_next(
			(OstProto::StreamControl::NextWhat) nextWhat); return true; }

	quint32 numPackets()
		{ return (quint32) mControl->num_packets(); }
	bool setNumPackets(quint32 numPackets)
		{ mControl->set_num_packets(numPackets); return true; }

	quint32 numBursts()
		{ return (quint32) mControl->num_bursts(); }
	bool setNumBursts(quint32 numBursts)
		{ mControl->set_num_bursts(numBursts); return true; }

	quint32 burstSize()
		{ return (quint32) mControl->packets_per_burst(); }
	bool setBurstSize(quint32 packetsPerBurst)
		{ mControl->set_packets_per_burst(packetsPerBurst); return true; }

	quint32 packetRate()
		{ return (quint32) mControl->packets_per_sec(); }
	bool setPacketRate(quint32 packetsPerSec)
		{ mControl->set_packets_per_sec(packetsPerSec); return true; }

	quint32 burstRate()
		{ return (quint32) mControl->bursts_per_sec(); }
	bool setBurstRate(quint32 burstsPerSec)
		{ mControl->set_bursts_per_sec(burstsPerSec); return true; }

	//---------------------------------------------------------------
	// Methods for use by Packet Model
	//---------------------------------------------------------------

	int numProtocols();

	//! Includes ALL protocol headers excluding payload data
	int protocolHeaderSize();
#if 0
	int protocolId(int index);
	int protocolIndex(int id);
#endif
	AbstractProtocol* protocol(int index);
private:
	QList<int>	selectedProtocols;
	int			mProtocolHeaderSize;
	void updateSelectedProtocols();

};

#endif
