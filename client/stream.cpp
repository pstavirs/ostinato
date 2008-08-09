#include <stream.h>

quint32 Stream::mAllocId = 0;

Stream::Stream()
{
	mId = mAllocId++;
	
	mCore = new OstProto::StreamCore;
	mMac = new MacProtocol;
	mIp = new IpProtocol;
#if 0
	// Default constructor
	InitDefaultMeta();
	InitDefaultProto();
	InitDefaultL2();
	InitDefaultL3();
	InitDefaultL4();
#endif
}

#if 0
void Stream::InitDefaultMeta()
{
	// TODO(LOW): Use #defines
	meta.patternMode = e_dp_fixed;
	meta.pattern = 0x00000000;
	meta.dataStartOfs = 0;	// FIXME(HIGH): this has to calculated
	meta.lenMode = e_fl_fixed;
	meta.frameLen = 64;
	meta.frameLenMin = 64;
	meta.frameLenMax = 1518;
}

void Stream::InitDefaultProto()
{
	// TODO(LOW): Use #defines
	proto.ft = e_ft_eth_2;
	proto.dsap = 0x00;
	proto.ssap = 0x00;
	proto.ctl  = 0x00;
	proto.ouiMsb = 0x00;
	proto.ouiLshw = 0x0000;
	
	proto.protoMask = PM_L3_PROTO_NONE | PM_L4_PROTO_NONE;
	proto.etherType = ETH_TYP_IP;
	proto.ipProto = IP_PROTO_TCP;
}


void Stream::InitDefaultL2()
{
	// TODO(LOW): Use #defines
	l2.eth.dstMacMshw = 0x0000;
	l2.eth.dstMacLsw  = 0x00000001;
	l2.eth.dstMacMode = e_mm_fixed;
	l2.eth.dstMacCount = 16;
	l2.eth.dstMacStep = 1;

	l2.eth.srcMacMshw = 0x0000;
	l2.eth.srcMacLsw  = 0x00000002;
	l2.eth.srcMacMode = e_mm_fixed;
	l2.eth.srcMacCount = 16;
	l2.eth.srcMacStep = 1;

	l2.eth.vlanMask = VM_UNTAGGED;
	l2.eth.ctpid = 0x8100;
	l2.eth.cvlanPrio = 0;
	l2.eth.cvlanCfi = 0;
	l2.eth.cvlanId = 2;
	l2.eth.stpid = 0x88a8;
	l2.eth.svlanPrio = 0;
	l2.eth.svlanCfi = 0;
	l2.eth.svlanId = 2;
}

void Stream::InitDefaultL3()
{
	InitDefaultL3Ip();
}

void Stream::InitDefaultL3Ip()
{
	l3.ip.ipMask = STREAM_DEF_IP_MASK;
	l3.ip.ver = STREAM_DEF_L3_IP_VER;
	l3.ip.hdrLen = STREAM_DEF_L3_IP_HDR_LEN;
	l3.ip.tos = STREAM_DEF_L3_IP_TOS;
	l3.ip.totLen = STREAM_DEF_L3_IP_TOT_LEN;
	l3.ip.id = STREAM_DEF_L3_IP_ID;
	l3.ip.flags = STREAM_DEF_L3_IP_FLAGS;
	l3.ip.fragOfs = STREAM_DEF_L3_IP_FRAG_OFS;
	l3.ip.ttl = STREAM_DEF_L3_IP_TTL;
	l3.ip.proto = STREAM_DEF_L3_IP_PROTO;
	l3.ip.cksum = STREAM_DEF_L3_IP_CKSUM;
	l3.ip.srcIp = STREAM_DEF_L3_IP_SRC_IP;
	l3.ip.srcIpMode = STREAM_DEF_L3_IP_SRC_IP_MODE;
	l3.ip.srcIpCount = STREAM_DEF_L3_IP_SRC_IP_COUNT;
	l3.ip.srcIpMask = STREAM_DEF_L3_IP_SRC_IP_MASK;
	l3.ip.dstIp = STREAM_DEF_L3_IP_DST_IP;
	l3.ip.dstIpMode = STREAM_DEF_L3_IP_DST_IP_MODE;
	l3.ip.dstIpCount = STREAM_DEF_L3_IP_DST_IP_COUNT;
	l3.ip.dstIpMask = STREAM_DEF_L3_IP_DST_IP_MASK;
}

void Stream::InitDefaultL4()
{
	InitDefaultL4Tcp();
	InitDefaultL4Udp();
}

void Stream::InitDefaultL4Tcp()
{
	l4.tcp.tcpMask = STREAM_DEF_L4_TCP_TCP_MASK;
	l4.tcp.srcPort = STREAM_DEF_L4_TCP_SRC_PORT;
	l4.tcp.dstPort = STREAM_DEF_L4_TCP_DST_PORT;
	l4.tcp.seqNum = STREAM_DEF_L4_TCP_SEQ_NUM;
	l4.tcp.ackNum = STREAM_DEF_L4_TCP_ACK_NUM;
	l4.tcp.hdrLen = STREAM_DEF_L4_TCP_HDR_LEN;
	l4.tcp.rsvd = STREAM_DEF_L4_TCP_RSVD;
	l4.tcp.flags = STREAM_DEF_L4_TCP_FLAGS;
	l4.tcp.window = STREAM_DEF_L4_TCP_WINDOW;
	l4.tcp.cksum = STREAM_DEF_L4_TCP_CKSUM;
	l4.tcp.urgPtr = STREAM_DEF_L4_TCP_URG_PTR;
}

void Stream::InitDefaultL4Udp()
{
	l4.udp.udpMask = STREAM_DEF_L4_UDP_UDP_MASK;
	l4.udp.srcPort = STREAM_DEF_L4_UDP_SRC_PORT;
	l4.udp.dstPort = STREAM_DEF_L4_UDP_DST_PORT;
	l4.udp.totLen = STREAM_DEF_L4_UDP_TOT_LEN;
	l4.udp.cksum = STREAM_DEF_L4_UDP_CKSUM;
}
#endif
