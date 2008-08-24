#include <QHostAddress>
#include "packetmodel.h"

PacketModel::PacketModel(Stream *pStream, QObject *parent)
{
	mpStream = pStream;
#ifdef NEW_IMPL
// Nothing else
#else
	populatePacketProtocols();

	registerFrameTypeProto();
	registerVlanProto();
	registerIpProto();
	registerArpProto();
	registerTcpProto();
	registerUdpProto();
	registerIcmpProto();
	registerIgmpProto();
	registerData();
	registerInvalidProto();
#endif
}

int PacketModel::rowCount(const QModelIndex &parent) const
{
	IndexId		parentId;

	// Parent == Invalid i.e. Invisible Root.
	// ==> Children are Protocol (Top Level) Items
	if (!parent.isValid())
#ifdef NEW_IMPL
		return mpStream->numProtocols();
#else
		return protoCount();
#endif

	// Parent - Valid Item
	parentId.w = parent.internalId();
	switch(parentId.ws.type)
	{
	case  ITYP_PROTOCOL:
#ifdef NEW_IMPL
		return mpStream->protocol(parentId.ws.protocol)->numFields();
#else
		return fieldCount(parentId.ws.protocol);
#endif
	case  ITYP_FIELD: 
		return 0;
	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

	Q_ASSERT(1 == 1);	// Unreachable code
	qWarning("%s: Catch all - need to investigate", __FUNCTION__);
	return 0; // catch all
}

int PacketModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QModelIndex PacketModel::index(int row, int col, const QModelIndex &parent) const
{
	QModelIndex	index;
	IndexId	id, parentId;

	if (!hasIndex(row, col, parent))
		goto _exit;

	// Parent is Invisible Root
	// Request for a Protocol Item
	if (!parent.isValid())
	{
		id.w = 0;
		id.ws.type = ITYP_PROTOCOL;
		id.ws.protocol = row;
		index = createIndex(row, col, id.w);
		goto _exit;
	}

	// Parent is a Valid Item
	parentId.w = parent.internalId();
	id.w = parentId.w;
	switch(parentId.ws.type)
	{
	case  ITYP_PROTOCOL:
		id.ws.type = ITYP_FIELD;
		index = createIndex(row, col, id.w);
		goto _exit;

	case  ITYP_FIELD: 
		goto _exit;

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

	Q_ASSERT(1 == 1);	// Unreachable code

_exit:
	return index;
}

QModelIndex PacketModel::parent(const QModelIndex &index) const
{
	QModelIndex	parentIndex;
	IndexId		id, parentId;

	if (!index.isValid())
		return QModelIndex();

	id.w = index.internalId();
	parentId.w = id.w;
	switch(id.ws.type)
	{
	case  ITYP_PROTOCOL:
		// return invalid index for invisible root
		goto _exit;

	case  ITYP_FIELD: 
		parentId.ws.type = ITYP_PROTOCOL;
		parentIndex = createIndex(id.ws.protocol, 0, parentId.w);
		goto _exit;

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

	Q_ASSERT(1 == 1); // Unreachable code

_exit:
	return parentIndex;
}

QVariant PacketModel::data(const QModelIndex &index, int role) const
{
	IndexId			id;
#ifdef NEW_IMPL
// Nothing
#else
	ProtocolInfo 	proto;
#endif

	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	id.w = index.internalId();
	switch(id.ws.type)
	{
	case  ITYP_PROTOCOL:
#ifdef NEW_IMPL
		return QString("%1 (%2)")
			.arg(mpStream->protocol(id.ws.protocol)->protocolShortName())
			.arg(mpStream->protocol(id.ws.protocol)->protocolName());
#else
		return protoName(id.ws.protocol);
#endif

	case  ITYP_FIELD: 
#ifdef NEW_IMPL
		return	mpStream->protocol(id.ws.protocol)->fieldName(index.row()) +
				QString(" : ") +
				mpStream->protocol(id.ws.protocol)->fieldTextValue(index.row());
#else
		return	fieldName(id.ws.protocol, index.row()) +
				QString(" : ") +
				fieldTextValue(id.ws.protocol, index.row()).toString();
#endif

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

	Q_ASSERT(1 == 1); // Unreachable code

	return QVariant();
}

#ifdef NEW_IMPL
// required methods all part of the Stream class
#else
/*
** --------------- Private Stuff -----------------
** FIXME(MED): Move these to the Stream Class
**
*/

/*!
  Looking at the stream's protocols and populate an ordered list of
  protocols accordingly. The order of protocols will be in the order of
  protocol headers viz.

  - None/Eth2/802.3 (Mac Addr)
  - LLC
  - SNAP
  - SVLAN
  - CVLAN
  - L3 (IP/ARP)
  - L4 (TCP/UDP/ICMP/IGMP)

*/
void PacketModel::populatePacketProtocols()
{
	int proto;

	// Clear the protocols list
	mPacketProtocols.clear();

	// Check and populate L2 Protocol
	switch(mpStream->frameType())
	{
		case Stream::e_ft_none:		
			proto = PTYP_L2_NONE;
			break;

		case Stream::e_ft_eth_2:
			proto = PTYP_L2_ETH_2;
			break;

		case Stream::e_ft_802_3_raw:
			proto = PTYP_L2_802_3_RAW;
			break;

		case Stream::e_ft_802_3_llc:
			mPacketProtocols.append(PTYP_L2_NONE);
			proto = PTYP_L2_802_3_LLC;
			break;

		case Stream::e_ft_snap:
			mPacketProtocols.append(PTYP_L2_NONE);
			mPacketProtocols.append(PTYP_L2_802_3_LLC);
			proto = PTYP_L2_SNAP;
			break;

		default:
			qDebug("%s: Unsupported frametype %d", __FUNCTION__,
				mpStream->frameType());
			proto = PTYP_INVALID;
	}
	mPacketProtocols.append(proto);

	// Check and populate VLANs, if present
	if (mpStream->vlan()->vlanFlags().testFlag(VlanProtocol::VlanSvlanTagged))
		mPacketProtocols.append(PTYP_SVLAN);

	if (mpStream->vlan()->vlanFlags().testFlag(VlanProtocol::VlanCvlanTagged))
		mPacketProtocols.append(PTYP_CVLAN);

	// Check and populate L3 protocols
	switch (mpStream->l3Proto())
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
				mpStream->l3Proto());
			proto = PTYP_INVALID;
	}
	mPacketProtocols.append(proto);

	// Check and populate L4 protocol
	switch(mpStream->l4Proto())
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
					mpStream->l4Proto());
			proto = PTYP_INVALID;
	};
	mPacketProtocols.append(proto);

_data:
	mPacketProtocols.append(PTYP_DATA);
}
/*!
  Returns the count of protocols in the current stream
*/
int PacketModel::protoCount() const
{
	return mPacketProtocols.count();
}

/*!
  Returns the count of fields in the given protocol
*/
int PacketModel::fieldCount(int protocol) const
{
	ProtocolInfo	proto;

	if (protocol >= mPacketProtocols.count())
		return 0;

	foreach(proto, mProtocols)
	{
		if (proto.handle == mPacketProtocols.at(protocol))
		{
			qDebug("proto=%d, name=%s",protocol,proto.name.toAscii().data());
			qDebug("fieldcount = %d", proto.fieldList.size());
			return proto.fieldList.count();
		}
	}

	return 0;
}

QString PacketModel::protoName(int protocol) const
{
	ProtocolInfo	proto;

	if (protocol >= mPacketProtocols.count())
		return 0;

	foreach(proto, mProtocols)
	{
		if (proto.handle == mPacketProtocols.at(protocol))
		{
			qDebug("proto=%d, name=%s",protocol,proto.name.toAscii().data());
			qDebug("fieldcount = %d", proto.fieldList.size());
			return proto.name;
		}
	}

	return QString();
}

QString PacketModel::fieldName(int protocol, int field) const
{
	ProtocolInfo	proto;

	if (protocol >= mPacketProtocols.count())
		return 0;

	foreach(proto, mProtocols)
	{
		if (proto.handle == mPacketProtocols.at(protocol))
		{
			qDebug("proto=%d, name=%s",protocol,proto.name.toAscii().data());
			qDebug("fieldcount = %d", proto.fieldList.size());
			if (field >= proto.fieldList.count())
				return QString();

			return proto.fieldList.at(field).name;
		}
	}

	return QString();
}

QVariant PacketModel::fieldTextValue(int protocol, int field) const
{
	if (protocol >= mPacketProtocols.count())
		return QVariant();

	switch(mPacketProtocols.at(protocol))
	{
	case PTYP_L2_NONE:
	case PTYP_L2_ETH_2:
		return ethField(field, FROL_TEXT_VALUE);
	case PTYP_L2_802_3_RAW:
		//return dot3Field(field, FROL_TEXT_VALUE); // FIXME(HIGH)	
		return ethField(field, FROL_TEXT_VALUE);
	case PTYP_L2_802_3_LLC:
		return llcField(field, FROL_TEXT_VALUE);
	case PTYP_L2_SNAP:
		return snapField(field, FROL_TEXT_VALUE);

	case PTYP_SVLAN:
		return svlanField(field, FROL_TEXT_VALUE);
	case PTYP_CVLAN:
		// return cvlanField(field, FROL_TEXT_VALUE);	// FIXME(HIGH)
		return svlanField(field, FROL_TEXT_VALUE);

	case PTYP_L3_IP:
		return ipField(field, FROL_TEXT_VALUE);
	case PTYP_L3_ARP:
		return QString();	// FIXME(HIGH)

	case PTYP_L4_TCP:
		return tcpField(field, FROL_TEXT_VALUE);
	case PTYP_L4_UDP:
		return udpField(field, FROL_TEXT_VALUE);
	case PTYP_L4_ICMP:
		return QString();	// FIXME(HIGH)
	case PTYP_L4_IGMP:
		return QString();	// FIXME(HIGH)

	case PTYP_INVALID:
		return QString();	// FIXME(HIGH)
	case PTYP_DATA:
		return QString();	// FIXME(HIGH)
	}

	return QString();
}

QVariant PacketModel::ethField(int field, int role) const
{
	FieldInfo	info;

	// FIXME(MED): Mac Addr formatting
	switch(field)
	{
	case 0:
		info.name = QString("Destination Mac Address");
		info.textValue = QString("%1").
			arg(mpStream->mac()->dstMac(), 12, BASE_HEX, QChar('0'));
		break;
	case 1:
		info.name = QString("Source Mac Address");
		info.textValue = QString("%1").
			arg(mpStream->mac()->srcMac(), 12, BASE_HEX, QChar('0'));
		break;
	case 2:
		info.name = QString("Type");
		info.textValue = QString("0x%1").
			arg(mpStream->eth2()->type(), 4, BASE_HEX, QChar('0'));
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}

QVariant PacketModel::llcField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("DSAP");
		info.textValue = QString("0x%1").
			arg(mpStream->llc()->dsap(), 2, BASE_HEX, QChar('0'));
		break;
	case 1:
		info.name = QString("SSAP");
		info.textValue = QString("0x%1").
			arg(mpStream->llc()->ssap(), 2, BASE_HEX, QChar('0'));
		break;
	case 2:
		info.name = QString("Control");
		info.textValue = QString("0x%1").
			arg(mpStream->llc()->ctl(), 2, BASE_HEX, QChar('0'));
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}

QVariant PacketModel::snapField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("OUI");
		info.textValue = QString("0x%1").
			arg(mpStream->snap()->oui(), 6, BASE_HEX, QChar('0'));
		break;
	case 1:
		info.name = QString("Type");
		info.textValue = QString("0x%1").
			arg(mpStream->eth2()->type(), 4, BASE_HEX, QChar('0'));
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}

QVariant PacketModel::svlanField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("TPID");
		info.textValue = QString("0x%1").
			arg(mpStream->vlan()->stpid(), 4, BASE_HEX, QChar('0'));
		break;
	case 1:
		info.name = QString("PCP");
		info.textValue = QString("%1").
			arg(mpStream->vlan()->svlanPrio());
		break;
	case 2:
		info.name = QString("DE");
		info.textValue = QString("%1").
			arg(mpStream->vlan()->svlanCfi());
		break;
	case 3:
		info.name = QString("VlanId");
		info.textValue = QString("%1").
			arg(mpStream->vlan()->svlanId());
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}


QVariant PacketModel::ipField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("Version");
		info.textValue = QString("%1").
			arg(mpStream->ip()->ver());
		break;
	case 1:
		info.name = QString("Header Length");
		info.textValue = QString("%1").
			arg(mpStream->ip()->hdrLen());
		break;
	case 2:
		info.name = QString("TOS/DSCP");
		info.textValue = QString("0x%1").
			arg(mpStream->ip()->tos(), 2, BASE_HEX, QChar('0'));
		break;
	case 3:
		info.name = QString("Total Length");
		info.textValue = QString("%1").
			arg(mpStream->ip()->totLen());
		break;
	case 4:
		info.name = QString("ID");
		info.textValue = QString("0x%1").
			arg(mpStream->ip()->id(), 2, BASE_HEX, QChar('0'));
		break;
	case 5:
		info.name = QString("Flags");
		info.textValue = QString("0x%1").
			arg(mpStream->ip()->flags(), 2, BASE_HEX, QChar('0')); // FIXME(HIGH)
		break;
	case 6:
		info.name = QString("Fragment Offset");
		info.textValue = QString("%1").
			arg(mpStream->ip()->fragOfs());
		break;
	case 7:
		info.name = QString("TTL");
		info.textValue = QString("%1").
			arg(mpStream->ip()->ttl());
		break;
	case 8:
		info.name = QString("Protocol Type");
		info.textValue = QString("0x%1").
			arg(mpStream->ip()->proto(), 2, BASE_HEX, QChar('0'));
		break;
	case 9:
		info.name = QString("Checksum");
		info.textValue = QString("0x%1").
			arg(mpStream->ip()->cksum(), 4, BASE_HEX, QChar('0'));
		break;
	case 10:
		info.name = QString("Source IP");
		info.textValue = QHostAddress(mpStream->ip()->srcIp()).toString();
		break;
	case 11:
		info.name = QString("Destination IP");
		info.textValue = QHostAddress(mpStream->ip()->dstIp()).toString();
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}


QVariant PacketModel::tcpField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("Source Port");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->srcPort());
		break;
	case 1:
		info.name = QString("Destination Port");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->dstPort());
		break;
	case 2:
		info.name = QString("Seq Number");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->seqNum());
		break;
	case 3:
		info.name = QString("Ack Number");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->ackNum());
		break;
	case 4:
		info.name = QString("Header Length");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->hdrLen());
		break;
	case 5:
		info.name = QString("Reserved");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->rsvd());
		break;
	case 6:
		info.name = QString("Flags");
		info.textValue = QString("0x%1").
			arg(mpStream->tcp()->flags(), 2, BASE_HEX, QChar('0'));
		break;
	case 7:
		info.name = QString("Window");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->flags());
		break;
	case 8:
		info.name = QString("Checksum");
		info.textValue = QString("0x%1").
			arg(mpStream->tcp()->cksum(), 4, BASE_HEX, QChar('0'));
		break;
	case 9:
		info.name = QString("Urgent Pointer");
		info.textValue = QString("%1").
			arg(mpStream->tcp()->urgPtr());
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}


QVariant PacketModel::udpField(int field, int role) const
{
	FieldInfo	info;

	switch(field)
	{
	case 0:
		info.name = QString("Source Port");
		info.textValue = QString("%1").
			arg(mpStream->udp()->srcPort());
		break;
	case 1:
		info.name = QString("Destination Port");
		info.textValue = QString("%1").
			arg(mpStream->udp()->dstPort());
		break;
	case 2:
		info.name = QString("Total Length");
		info.textValue = QString("%1").
			arg(mpStream->udp()->totLen());
		break;
	case 3:
		info.name = QString("Checksum");
		info.textValue = QString("0x%1").
			arg(mpStream->udp()->cksum(), 4, BASE_HEX, QChar('0'));
		break;
	default:
		info.name = QString();
		info.textValue = QString();
	}

	switch(role)
	{
	case FROL_NAME:
		return info.name;
	case FROL_TEXT_VALUE:
		return info.textValue;
	default:
		;
	}
	
	Q_ASSERT(1 == 1); // Unreachable code
	return QVariant();
}



/*
** ------------- Registration Functions ---------------
*/

void PacketModel::registerProto(uint handle, char *name, char *abbr)
{
	ProtocolInfo	proto;

	proto.handle = handle;
	proto.name = QString(name);
	proto.abbr = QString(abbr);
	mProtocols.append(proto);
}

void PacketModel::registerField(uint protoHandle, char *name, char *abbr)
{
	for (int i = 0; i < mProtocols.size(); i++)
	{
		if (mProtocols.at(i).handle == protoHandle)
		{
			FieldInfo	field;

			field.name = QString(name);
			field.abbr = QString(abbr);
			mProtocols[i].fieldList.append(field);
			qDebug("proto = %d, name = %s", protoHandle, name);
			break;
		}
	}
}

void PacketModel::registerFrameTypeProto()
{
	registerProto(PTYP_L2_NONE, "None", "");
	registerField(PTYP_L2_NONE, "Destination Mac", "dstMac");
	registerField(PTYP_L2_NONE, "Source Mac", "srcMac");

	registerProto(PTYP_L2_ETH_2, "Ethernet II", "eth");
	registerField(PTYP_L2_ETH_2, "Destination Mac", "dstMac");
	registerField(PTYP_L2_ETH_2, "Source Mac", "srcMac");
	registerField(PTYP_L2_ETH_2, "Ethernet Type", "type");

	registerProto(PTYP_L2_802_3_RAW, "IEEE 802.3 Raw", "dot3raw");
	registerField(PTYP_L2_802_3_RAW, "Destination Mac", "dstMac");
	registerField(PTYP_L2_802_3_RAW, "Source Mac", "srcMac");
	registerField(PTYP_L2_802_3_RAW, "Length", "len");

	registerProto(PTYP_L2_802_3_LLC, "802.3 LLC", "dot3llc");
	registerField(PTYP_L2_802_3_LLC, "Destination Service Acces Point", "dsap");
	registerField(PTYP_L2_802_3_LLC, "Source Service Acces Point", "ssap");
	registerField(PTYP_L2_802_3_LLC, "Control", "ctl");

	registerProto(PTYP_L2_SNAP, "SNAP", "dot3snap");
	registerField(PTYP_L2_SNAP, "Organisationally Unique Identifier", "oui");
	registerField(PTYP_L2_SNAP, "Type", "type");

}

void PacketModel::registerVlanProto()
{
	registerProto(PTYP_SVLAN, "IEEE 802.1ad Service VLAN", "SVLAN");

	registerField(PTYP_SVLAN, "Tag Protocol Identifier", "tpid");
	registerField(PTYP_SVLAN, "Priority Code Point", "pcp");
	registerField(PTYP_SVLAN, "Drop Eligible", "de");
	registerField(PTYP_SVLAN, "VLAN Identifier", "vlan");

	registerProto(PTYP_CVLAN, "IEEE 802.1Q VLAN/CVLAN", "VLAN");

	registerField(PTYP_CVLAN, "Tag Protocol Identifier", "tpid");
	registerField(PTYP_CVLAN, "Priority", "prio");
	registerField(PTYP_CVLAN, "Canonical Format Indicator", "cfi");
	registerField(PTYP_CVLAN, "VLAN Identifier", "vlan");
}

void PacketModel::registerIpProto()
{
	registerProto(PTYP_L3_IP, "Internet Protocol version 4", "IPv4");

	registerField(PTYP_L3_IP, "Version", "ver");
	registerField(PTYP_L3_IP, "Header Length", "hdrlen");
	registerField(PTYP_L3_IP, "Type of Service/DiffServ Code Point", "tos");
	registerField(PTYP_L3_IP, "Total Length", "len");
	registerField(PTYP_L3_IP, "Identification", "id");
	registerField(PTYP_L3_IP, "Flags", "flags");
	registerField(PTYP_L3_IP, "Fragment Offset", "fragofs");
	registerField(PTYP_L3_IP, "Time to Live", "ttl");
	registerField(PTYP_L3_IP, "Protocol Id", "proto");
	registerField(PTYP_L3_IP, "Checksum", "cksum");
	registerField(PTYP_L3_IP, "Source IP", "srcip");
	registerField(PTYP_L3_IP, "Destination IP", "dstip");
}

void PacketModel::registerArpProto()
{
	// TODO (LOW)
}

void PacketModel::registerTcpProto()
{
	registerProto(PTYP_L4_TCP, "Transmission Control Protocol", "TCP");

	registerField(PTYP_L4_TCP, "Source Port", "srcport");
	registerField(PTYP_L4_TCP, "Destination Port", "dstport");
	registerField(PTYP_L4_TCP, "Sequence Number", "seqnum");
	registerField(PTYP_L4_TCP, "Acknowledgement Number", "acknum");
	registerField(PTYP_L4_TCP, "Header Length", "hdrlen");
	registerField(PTYP_L4_TCP, "Reserved", "rsvd");
	registerField(PTYP_L4_TCP, "Flags", "flags");
	registerField(PTYP_L4_TCP, "Window", "win");
	registerField(PTYP_L4_TCP, "Checksum", "cksum");
	registerField(PTYP_L4_TCP, "Urgent Pointer", "urgptr");
}

void PacketModel::registerUdpProto()
{
	registerProto(PTYP_L4_UDP, "User Datagram Protocol", "UDP");

	registerField(PTYP_L4_UDP, "Source Port", "srcport");
	registerField(PTYP_L4_UDP, "Destination Port", "dstport");
	registerField(PTYP_L4_UDP, "Length", "len");
	registerField(PTYP_L4_UDP, "Checksum", "cksum");
}

void PacketModel::registerIcmpProto()
{
	// TODO (LOW)
}

void PacketModel::registerIgmpProto()
{
	// TODO (LOW)
}

void PacketModel::registerInvalidProto()
{
	registerProto(PTYP_INVALID, "Invalid Protocol (bug in code)", "invalid");
}

void PacketModel::registerData()
{
	registerProto(PTYP_DATA, "Data", "data");
}

#endif
