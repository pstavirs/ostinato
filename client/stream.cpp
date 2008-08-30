#include <qendian.h>
#include <QHostAddress>

#include "stream.h"

#include <google/protobuf/message.h>

#define BASE_HEX		16

QString PayloadProtocol::fieldTextValue(int index)
{
	int len;
	quint32 pat;
	QString	textValue;

	if (parentStream)
	{
		len = parentStream->frameLen() - parentStream->protocolHeaderSize();
		pat = parentStream->pattern();
	}
	else
	{
		len = 1500;		// FIXME(HI): testing only
		pat = 0x0a0b0c0d;
	}

	// Make a larger string and then resize to the correct size to
	// take care of the case where len is not a multiple of pattern size
	if (len > 0)
	{
		// TODO(LOW): allow non-4byte patterns!!!
		int w = 4;	// data pattern size

		for (int i = 0; i < (len/w + 1); i++)
			textValue.append(QString("%1").arg(
				pat, w*2, BASE_HEX, QChar('0')));
		textValue.resize(len);
	}

	return textValue;
}

QByteArray PayloadProtocol::fieldRawValue(int index)
{
	int len;
	quint32 pat;
	QByteArray	rawValue;

	if (parentStream)
	{
		len = parentStream->frameLen() - parentStream->protocolHeaderSize();
		pat = parentStream->pattern();
	}
	else
	{
		len = 1500;		// FIXME(HI): testing only
		pat = 0x0a0b0c0d;
	}

	// Make a larger byteArray and then resize to the correct size to
	// take care of the case where len is not a multiple of pattern size
	if (len > 0)
	{
		// TODO(LOW): allow non-4byte patterns!!!
		int w = 4;	// data pattern size

		rawValue.resize(len + 4);
		for (int i = 0; i < (len/w + 1); i++)
			qToBigEndian(pat, (uchar*) (rawValue.data() + i*sizeof(pat)));
		rawValue.resize(len);
	}

	return rawValue;
}

QString MacProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Destination Mac Address");
		break;
	case 1:
		name = QString("Source Mac Address");
		break;
	default:
		name = QString();
		break;
	}

	return name;
}

QString MacProtocol::fieldTextValue(int index)
{
	QString textValue;

	// FIXME(MED): Mac Addr formatting
	switch(index)
	{
	case 0:
		textValue = QString("%1").
			arg(dstMac(), 12, BASE_HEX, QChar('0'));
		break;
	case 1:
		textValue = QString("%1").
			arg(srcMac(), 12, BASE_HEX, QChar('0'));
		break;
	default:
		textValue = QString();
		break;
	}

	return textValue;
}

QByteArray MacProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(8);
		qToBigEndian(dstMac(), (uchar *) rawValue.data());
		rawValue.remove(0, 2);
		qDebug("dstMac(%d): %s", rawValue.size(), rawValue.toHex().constData());
		break;
	case 1:
		rawValue.resize(8);
		qToBigEndian(srcMac(), (uchar *) rawValue.data());
		rawValue.remove(0, 2);
		qDebug("srcMac(%d): %s", rawValue.size(), rawValue.toHex().constData());
		break;
	default:
		break;
	}

	return rawValue;
}

QString LlcProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("DSAP");
		break;
	case 1:
		name = QString("SSAP");
		break;
	case 2:
		name = QString("Control");
		break;
	default:
		name = QString();
		break;
	}

	return name;
}

QString LlcProtocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("0x%1").
			arg(dsap(), 2, BASE_HEX, QChar('0'));
		break;
	case 1:
		textValue = QString("0x%1").
			arg(ssap(), 2, BASE_HEX, QChar('0'));
		break;
	case 2:
		textValue = QString("0x%1").
			arg(ctl(), 2, BASE_HEX, QChar('0'));
		break;
	default:
		textValue = QString();
		break;
	}

	return textValue;
}

QByteArray LlcProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(1);
		rawValue[0] = dsap();
		break;
	case 1:
		rawValue.resize(1);
		rawValue[0] = ssap();
		break;
	case 2:
		rawValue.resize(1);
		rawValue[0] = ctl();
		break;
	default:
		break;
	}

	return rawValue;
}

QString SnapProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("OUI");
		break;
	default:
		name = QString();
		break;
	}

	return name;
}

QString SnapProtocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("0x%1").
			arg(oui(), 6, BASE_HEX, QChar('0'));
		break;
	default:
		textValue = QString();
		break;
	}

	return textValue;
}

QByteArray SnapProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(4);
		qToBigEndian(oui(), (uchar *) rawValue.data());
		rawValue.remove(0, 1);
		break;
	default:
		break;
	}

	return rawValue;
}

QString Eth2Protocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Type");
		break;
	default:
		name = QString();
		break;
	}
	return name;
}

QString Eth2Protocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("0x%1").
			arg(type(), 4, BASE_HEX, QChar('0'));
		break;
	default:
		textValue = QString();
		break;
	}
	return textValue;
}

QByteArray Eth2Protocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(2);
		qToBigEndian(type(), (uchar*) rawValue.data());
		break;
	default:
		break;
	}

	return rawValue;
}

QString Dot3Protocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Length");
		break;
	default:
		name = QString();
		break;
	}
	return name;
}

QString Dot3Protocol::fieldTextValue(int index)
{ 
	if (parentStream) 
		return QString("%1").arg(parentStream->frameLen());
	else 
		return QString("00");		
}

QByteArray Dot3Protocol::fieldRawValue(int index)
{
	QByteArray	ba;

	if (parentStream) 
		qToBigEndian((quint16) parentStream->frameLen(), (uchar*) ba.data());
	else
	{
		ba.resize(2);
		ba[0] = ba[1] = 0;
	}

	return ba;
}

int	VlanProtocol::numFields()
{
	if (isSingleTagged())
		return 4;
	else if (isDoubleTagged())
		return 8;
	else 
	{	
		Q_ASSERT(isUntagged());
		return 0;
	}
}

QString VlanProtocol::fieldName(int index)
{
	QString name;

	if (isDoubleTagged())
	{
		switch(index)
		{
		case 0:
			name = QString("TPID");
			break;
		case 1:
			name = QString("PCP");
			break;
		case 2:
			name = QString("DE");
			break;
		case 3:
			name = QString("VlanId");
			break;
		default:
			index -= 4;
			goto _single_tag;
		}

		goto _exit;
	}

_single_tag:
	switch(index)
	{
	case 0:
		name = QString("TPID");
		break;
	case 1:
		name = QString("Priority");
		break;
	case 2:
		name = QString("CFI");
		break;
	case 3:
		name = QString("VlanId");
		break;
	default:
		name = QString();
		break;
	}

_exit:
	return name;
}

QString VlanProtocol::fieldTextValue(int index)
{
	QString textValue;

	if (isDoubleTagged())
	{
		switch(index)
		{
		case 0:
			textValue = QString("0x%1").
				arg(stpid(), 4, BASE_HEX, QChar('0'));
			break;
		case 1:
			textValue = QString("%1").
				arg(svlanPrio());
			break;
		case 2:
			textValue = QString("%1").
				arg(svlanCfi());
			break;
		case 3:
			textValue = QString("%1").
				arg(svlanId());
			break;
		default:
			index -= 4;
			goto _single_tag;
		}

		goto _exit;
	}

_single_tag:
	switch(index)
	{
	case 0:
		textValue = QString("0x%1").
			arg(ctpid(), 4, BASE_HEX, QChar('0'));
		break;
	case 1:
		textValue = QString("%1").
			arg(cvlanPrio());
		break;
	case 2:
		textValue = QString("%1").
			arg(cvlanCfi());
		break;
	case 3:
		textValue = QString("%1").
			arg(cvlanId());
		break;
	default:
		textValue = QString();
		break;
	}

_exit:
	return textValue;
}

QByteArray VlanProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	if (isDoubleTagged())
	{
		switch(index)
		{
		case 0:
			rawValue.resize(2);
			qToBigEndian(stpid(), (uchar*) rawValue.data());
			break;
		case 1:
			rawValue.resize(2);
			qToBigEndian((svlanPrio() << 13) | (svlanCfi() < 12) | svlanId(),
					(uchar*) rawValue.data());
			break;
		case 2:
			// Combined with prio above
			break;
		case 3:
			// Combined with prio above
			break;
		default:
			index -= 4;
			goto _single_tag;
		}

		goto _exit;
	}

_single_tag:
	switch(index)
	{
	case 0:
		rawValue.resize(2);
		qToBigEndian(ctpid(), (uchar*) rawValue.data());
		break;
	case 1:
		rawValue.resize(2);
		qToBigEndian((cvlanPrio() << 13) | (cvlanCfi() < 12) | cvlanId(),
		   	(uchar*) rawValue.data());
		break;
	case 2:
		// Combined with prio above
		break;
	case 3:
		// Combined with prio above
		break;
	default:
		break;
	}

_exit:
	return rawValue;
}

QString IpProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Version");
		break;
	case 1:
		name = QString("Header Length");
		break;
	case 2:
		name = QString("TOS/DSCP");
		break;
	case 3:
		name = QString("Total Length");
		break;
	case 4:
		name = QString("ID");
		break;
	case 5:
		name = QString("Flags");
		break;
	case 6:
		name = QString("Fragment Offset");
		break;
	case 7:
		name = QString("TTL");
		break;
	case 8:
		name = QString("Protocol Type");
		break;
	case 9:
		name = QString("Checksum");
		break;
	case 10:
		name = QString("Source IP");
		break;
	case 11:
		name = QString("Destination IP");
		break;
	default:
		name = QString();
	}

	return name;
}

QString IpProtocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("%1").
			arg(ver());
		break;
	case 1:
		textValue = QString("%1").
			arg(hdrLen());
		break;
	case 2:
		textValue = QString("0x%1").
			arg(tos(), 2, BASE_HEX, QChar('0'));
		break;
	case 3:
		textValue = QString("%1").
			arg(totLen());
		break;
	case 4:
		textValue = QString("0x%1").
			arg(id(), 2, BASE_HEX, QChar('0'));
		break;
	case 5:
		textValue = QString("0x%1").
			arg(flags(), 2, BASE_HEX, QChar('0')); // FIXME(MED): bitmap?
		break;
	case 6:
		textValue = QString("%1").
			arg(fragOfs());
		break;
	case 7:
		textValue = QString("%1").
			arg(ttl());
		break;
	case 8:
		textValue = QString("0x%1").
			arg(proto(), 2, BASE_HEX, QChar('0'));
		break;
	case 9:
		textValue = QString("0x%1").
			arg(cksum(), 4, BASE_HEX, QChar('0'));
		break;
	case 10:
		textValue = QHostAddress(srcIp()).toString();
		break;
	case 11:
		textValue = QHostAddress(dstIp()).toString();
		break;
	default:
		textValue = QString();
	}

	return textValue;
}

QByteArray IpProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(1);
		//qToBigEndian((ver() << 4) | hdrLen(), (uchar*) rawValue.data());
		rawValue[0]=(ver() << 4) | hdrLen();
		break;
	case 1:
		// Combined with previous 4 bits of ver!!
		break;
	case 2:
		rawValue.resize(1);
		qToBigEndian(tos(), (uchar*) rawValue.data());
		break;
	case 3:
		rawValue.resize(2);
		qToBigEndian(totLen(), (uchar*) rawValue.data());
		break;
	case 4:
		rawValue.resize(2);
		qToBigEndian(id(), (uchar*) rawValue.data());
		break;
	case 5:
		rawValue.resize(2);
		qToBigEndian((quint16)((flags() << 13) | fragOfs()),
				(uchar*) rawValue.data());
		break;
	case 6:
		// Combined with previous 3 bits of flags!!
		break;
	case 7:
		rawValue.resize(1);
		qToBigEndian(ttl(), (uchar*) rawValue.data());
		break;
	case 8:
		rawValue.resize(1);
		qToBigEndian(proto(), (uchar*) rawValue.data());
		break;
	case 9:
		rawValue.resize(2);
		qToBigEndian(cksum(), (uchar*) rawValue.data());
		break;
	case 10:
		rawValue.resize(4);
		qToBigEndian(srcIp(), (uchar*) rawValue.data());
		break;
	case 11:
		rawValue.resize(4);
		qToBigEndian(dstIp(), (uchar*) rawValue.data());
		break;
	default:
		break;
	}

	return rawValue;
}

QString TcpProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Source Port");
		break;
	case 1:
		name = QString("Destination Port");
		break;
	case 2:
		name = QString("Seq Number");
		break;
	case 3:
		name = QString("Ack Number");
		break;
	case 4:
		name = QString("Header Length");
		break;
	case 5:
		name = QString("Reserved");
		break;
	case 6:
		name = QString("Flags");
		break;
	case 7:
		name = QString("Window");
		break;
	case 8:
		name = QString("Checksum");
		break;
	case 9:
		name = QString("Urgent Pointer");
		break;
	default:
		name = QString();
	}

	return name;
}

QString TcpProtocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("%1").
			arg(srcPort());
		break;
	case 1:
		textValue = QString("%1").
			arg(dstPort());
		break;
	case 2:
		textValue = QString("%1").
			arg(seqNum());
		break;
	case 3:
		textValue = QString("%1").
			arg(ackNum());
		break;
	case 4:
		textValue = QString("%1").
			arg(hdrLen());
		break;
	case 5:
		textValue = QString("%1").
			arg(rsvd());
		break;
	case 6:
		textValue = QString("0x%1").
			arg(flags(), 2, BASE_HEX, QChar('0'));
		break;
	case 7:
		textValue = QString("%1").
			arg(window(), 2, BASE_HEX, QChar('0'));
		break;
	case 8:
		textValue = QString("0x%1").
			arg(cksum(), 4, BASE_HEX, QChar('0'));
		break;
	case 9:
		textValue = QString("%1").
			arg(urgPtr());
		break;
	default:
		textValue = QString();
	}

	return textValue;
}

QByteArray TcpProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(2);
		qToBigEndian(srcPort(), (uchar*) rawValue.data());
		break;
	case 1:
		rawValue.resize(2);
		qToBigEndian(dstPort(), (uchar*) rawValue.data());
		break;
	case 2:
		rawValue.resize(4);
		qToBigEndian(seqNum(), (uchar*) rawValue.data());
		break;
	case 3:
		rawValue.resize(4);
		qToBigEndian(ackNum(), (uchar*) rawValue.data());
		break;
	case 4:
		rawValue.resize(1);
		rawValue[0] = (hdrLen() << 4) | rsvd();
		break;
	case 5:
		// Combined with hdrLen above
		break;
	case 6:
		rawValue.resize(1);
		rawValue[0] = flags();
		break;
	case 7:
		rawValue.resize(2);
		qToBigEndian(window(), (uchar*) rawValue.data());
		break;
	case 8:
		rawValue.resize(2);
		qToBigEndian(cksum(), (uchar*) rawValue.data());
		break;
	case 9:
		rawValue.resize(2);
		qToBigEndian(urgPtr(), (uchar*) rawValue.data());
		break;
	default:
		break;
	}

	return rawValue;
}

QString UdpProtocol::fieldName(int index)
{
	QString name;

	switch(index)
	{
	case 0:
		name = QString("Source Port");
		break;
	case 1:
		name = QString("Destination Port");
		break;
	case 2:
		name = QString("Total Length");
		break;
	case 3:
		name = QString("Checksum");
		break;
	default:
		name = QString();
	}

	return name;
}

QString UdpProtocol::fieldTextValue(int index)
{
	QString textValue;

	switch(index)
	{
	case 0:
		textValue = QString("%1").
			arg(srcPort());
		break;
	case 1:
		textValue = QString("%1").
			arg(dstPort());
		break;
	case 2:
		textValue = QString("%1").
			arg(totLen());
		break;
	case 3:
		textValue = QString("0x%1").
			arg(cksum(), 4, BASE_HEX, QChar('0'));
		break;
	default:
		textValue = QString();
	}

	return textValue;
}

QByteArray UdpProtocol::fieldRawValue(int index)
{
	QByteArray rawValue;

	switch(index)
	{
	case 0:
		rawValue.resize(2);
		qToBigEndian(srcPort(), (uchar*) rawValue.data());
		break;
	case 1:
		rawValue.resize(2);
		qToBigEndian(dstPort(), (uchar*) rawValue.data());
		break;
	case 2:
		rawValue.resize(2);
		qToBigEndian(totLen(), (uchar*) rawValue.data());
		break;
	case 3:
		rawValue.resize(2);
		qToBigEndian(cksum(), (uchar*) rawValue.data());
		break;
	default:
		break;
	}

	return rawValue;
}



//-----------------------------------------------------
// Stream Class Methods
//-----------------------------------------------------


Stream::Stream()
{
	mId = 0xFFFFFFFF;

	mCore = new OstProto::StreamCore;

//	mCore->set_port_id(0xFFFFFFFF);
//	mCore->set_stream_id(mId);

	mUnknown = new UnknownProtocol;
	mPayload = new PayloadProtocol(); //FIXME(MED): need to pass parent stream

	mMac = new MacProtocol;

	mLlc = new LlcProtocol;
	mSnap = new SnapProtocol;
	mEth2 = new Eth2Protocol;
	mDot3 = new Dot3Protocol();		// FIXME(MED): need to pass parent stream
	mVlan = new VlanProtocol;

	mIp = new IpProtocol;
	mArp = new ArpProtocol;

	mTcp = new TcpProtocol;
	mUdp = new UdpProtocol;
	mIcmp = new IcmpProtocol;
	mIgmp = new IgmpProtocol;
}

void Stream::getConfig(uint portId, OstProto::Stream *s)
{
	s->mutable_stream_id()->set_id(mId);

	s->mutable_core()->CopyFrom(*mCore);

	mMac->getConfig(s->mutable_mac());
	mMac->getConfig(s->mutable_mac());
	mLlc->getConfig(s->mutable_llc());
	mSnap->getConfig(s->mutable_snap());
	mEth2->getConfig(s->mutable_eth2());
	mVlan->getConfig(s->mutable_vlan());

	mIp->getConfig(s->mutable_ip());
	mArp->getConfig(s->mutable_arp());

	mTcp->getConfig(s->mutable_tcp());
	mUdp->getConfig(s->mutable_udp());
	mIcmp->getConfig(s->mutable_icmp());
	mIgmp->getConfig(s->mutable_igmp());
}

// FIXME(HIGH): Replace this by some Protocol Registration mechanism at Init
#define PTYP_L2_NONE		1
#define PTYP_L2_ETH_2		2
#define PTYP_L2_802_3_RAW	3

#define PTYP_L2_802_3_LLC	4
#define PTYP_L2_SNAP		5

#define PTYP_VLAN			10

#define PTYP_L3_IP			30
#define PTYP_L3_ARP			31

#define PTYP_L4_TCP			40	
#define PTYP_L4_UDP			41	
#define PTYP_L4_ICMP		42
#define PTYP_L4_IGMP		43

#define PTYP_INVALID		0
#define PTYP_DATA			0xFF

void Stream::updateSelectedProtocols()
{
	int proto;

	// Clear the selected protocols list
	selectedProtocols.clear();

	// Check and populate L2 Protocol
	switch(frameType())
	{
		case Stream::e_ft_none:		
			proto = PTYP_L2_NONE;
			break;

		case Stream::e_ft_eth_2:
			selectedProtocols.append(PTYP_L2_NONE);
			proto = PTYP_L2_ETH_2;
			break;

		case Stream::e_ft_802_3_raw:
			selectedProtocols.append(PTYP_L2_NONE);
			proto = PTYP_L2_802_3_RAW;
			break;

		case Stream::e_ft_802_3_llc:
			selectedProtocols.append(PTYP_L2_NONE);
			proto = PTYP_L2_802_3_LLC;
			break;

		case Stream::e_ft_snap:
			selectedProtocols.append(PTYP_L2_NONE);
			selectedProtocols.append(PTYP_L2_802_3_LLC);
			proto = PTYP_L2_SNAP;
			break;

		default:
			qDebug("%s: Unsupported frametype %d", __FUNCTION__,
				frameType());
			proto = PTYP_INVALID;
	}
	selectedProtocols.append(proto);

	// Check and populate VLANs, if present
	if (!vlan()->isUntagged())
		selectedProtocols.append(PTYP_VLAN);

	// Check and populate L3 protocols
	switch (l3Proto())
	{
		case Stream::e_l3_none :
			goto _data;
			break;

		case Stream::e_l3_ip :
			proto = PTYP_L3_IP;
			break;

		case Stream::e_l3_arp:
			proto = PTYP_L3_ARP;
			break;

		default:
			qDebug("%s: Unsupported L3 Proto %d", __FUNCTION__, 
				l3Proto());
			proto = PTYP_INVALID;
	}
	selectedProtocols.append(proto);

	// Check and populate L4 protocol
	switch(l4Proto())
	{
		case Stream::e_l4_none:
			goto _data;
			break;
		case Stream::e_l4_tcp:
			proto = PTYP_L4_TCP;	
			break;
		case Stream::e_l4_udp:
			proto = PTYP_L4_UDP;	
			break;
		case Stream::e_l4_icmp:
			proto = PTYP_L4_ICMP;	
			break;
		case Stream::e_l4_igmp:
			proto = PTYP_L4_IGMP;	
			break;
		default:
			qDebug("%s: Unsupported L4 Proto %d", __FUNCTION__,
					l4Proto());
			proto = PTYP_INVALID;
	};
	selectedProtocols.append(proto);

_data:

	mProtocolHeaderSize = 0;
#ifndef SRIVATSP
#if 0
	for (int i = 0; i < selectedProtocols.size(); i++)
		mProtocolHeaderSize += protocol(i)->protocolRawValue().size();
#endif
#endif
	selectedProtocols.append(PTYP_DATA);
}

int Stream::protocolHeaderSize()
{
	updateSelectedProtocols();			// FIXME(HI): shd not happen everytime
	return mProtocolHeaderSize;
}

int Stream::numProtocols()
{
	updateSelectedProtocols();			// FIXME(HI): shd not happen everytime
	return selectedProtocols.size();
}

#if 0
int Stream::protocolId(int index)
{
	updateSelectedProtocols();			// FIXME(HI): shd not happen everytime
	if (index < selectedProtocols.size())
		return selectedProtocols.at(index);
	else
		return -1;
}
int Stream::protocolIndex(int id)
{
	
}
#endif

AbstractProtocol* Stream::protocol(int index)
{
	int id;

	updateSelectedProtocols();			// FIXME(HI): shd not happen everytime

	id = selectedProtocols.at(index);
	
	switch(id)
	{
		case PTYP_L2_NONE:
			return mac();
		case PTYP_L2_ETH_2:
			return eth2();
		case PTYP_L2_802_3_RAW:
			return dot3();

		case PTYP_L2_802_3_LLC:
			return llc();

		case PTYP_L2_SNAP:
			return snap();

		case PTYP_VLAN:
			return vlan();

		case PTYP_L3_IP:
			return ip();
		case PTYP_L3_ARP:
			return arp();

		case PTYP_L4_TCP:	
			return tcp();
		case PTYP_L4_UDP:	
			return udp();
		case PTYP_L4_ICMP:
			return icmp();
		case PTYP_L4_IGMP:
			return igmp();

		case PTYP_INVALID:
			return mUnknown;	
		case PTYP_DATA:
			return mPayload;
		default:
			return mUnknown;	
	}
}

