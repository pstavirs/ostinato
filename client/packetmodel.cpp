#include "packetmodel.h"

PacketModel::PacketModel(Stream *pStream, QObject *parent)
{
	mpStream = pStream;
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
}

int PacketModel::rowCount(const QModelIndex &parent) const
{
	IndexId		parentId;

	// Parent - Invalid i.e. Invisible Root.
	// Children - Protocol (Top Level) Items
	if (!parent.isValid())
		return mPacketProtocols.count();

	// Parent - Valid Item
	parentId.w = parent.internalId();
	switch(parentId.ws.type)
	{
	case  ITYP_PROTOCOL:
		return fieldCount(parentId.ws.protocol);
	case  ITYP_FIELD: 
		return subfieldCount(parentId.ws.protocol, parentId.ws.field);
	case  ITYP_SUBFIELD:
		return 0;
	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

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
		id.ws.protocol = mPacketProtocols.at(row);
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
		id.ws.field = row;
		index = createIndex(row, col, id.w);
		goto _exit;

	case  ITYP_FIELD: 
		// TODO(MED): Nothing till subfield support is added
		goto _exit;

	case  ITYP_SUBFIELD:
		goto _exit;

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

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
		parentId.ws.field = 0;
		parentIndex = createIndex(mPacketProtocols.indexOf(parentId.ws.protocol), 0, 
			parentId.w);
		goto _exit;

	case  ITYP_SUBFIELD:
		// TODO(MED): invalid index till subfield support is added
		goto _exit;

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

_exit:
	return parentIndex;
}

QVariant PacketModel::data(const QModelIndex &index, int role) const
{
	IndexId			id;
	ProtocolInfo 	proto;

	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	id.w = index.internalId();
	foreach(proto, mProtocols)
	{
		if (proto.handle == id.ws.protocol)
			goto _found;
	}
	return QVariant();

_found:
	switch(id.ws.type)
	{
	case  ITYP_PROTOCOL:
		return proto.name;

	case  ITYP_FIELD: 
		return	proto.fieldList.at(id.ws.field).name;

	case  ITYP_SUBFIELD:
		return QVariant();	// TODO(MED): Till subfield support is added

	default:
		qWarning("%s: Unhandled ItemType", __FUNCTION__);
	}

	return QVariant();
}


/*
** --------------- Private Stuff -----------------
*/
void PacketModel::populatePacketProtocols()
{
	int proto;

	// Clear the protocols list
	mPacketProtocols.clear();

	// Check and populate L2 Protocol
	switch(mpStream->proto.ft)
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
			proto = PTYP_L2_802_3_LLC;
			break;

		case Stream::e_ft_snap:
			proto = PTYP_L2_SNAP;
			break;

		default:
			qDebug("%s: Unsupported frametype", __FUNCTION__);
			proto = PTYP_INVALID;
	}
	mPacketProtocols.append(proto);

	// Check and populate VLANs, if present
	if (mpStream->l2.eth.vlanMask & VM_SVLAN_TAGGED)
		mPacketProtocols.append(PTYP_SVLAN);

	if (mpStream->l2.eth.vlanMask & VM_CVLAN_TAGGED)
		mPacketProtocols.append(PTYP_CVLAN);

	// Check and populate L3 protocols
	if (mpStream->proto.protoMask & PM_L3_PROTO_NONE)
		goto _data;

	switch(mpStream->proto.etherType)
	{
		case ETH_TYP_IP:
			proto = PTYP_L3_IP;
			break;

		case ETH_TYP_ARP:
			proto = PTYP_L3_ARP;
			break;

		default:
			qDebug("%s: Unsupported ethtype", __FUNCTION__);
			proto = PTYP_INVALID;
	}
	mPacketProtocols.append(proto);

	if (mpStream->proto.protoMask & PM_L4_PROTO_NONE)
		goto _data;

	switch(mpStream->proto.ipProto)
	{
		case IP_PROTO_TCP:
			proto = PTYP_L4_TCP;	
			break;
		case IP_PROTO_UDP:
			proto = PTYP_L4_UDP;	
			break;
		case IP_PROTO_ICMP:
			proto = PTYP_L4_ICMP;	
			break;
		case IP_PROTO_IGMP:
			proto = PTYP_L4_IGMP;	
			break;
		default:
			qDebug("%s: Unsupported ipProto", __FUNCTION__);
			proto = PTYP_INVALID;
	};
	mPacketProtocols.append(proto);

_data:
	mPacketProtocols.append(PTYP_DATA);
}

int PacketModel::fieldCount(uint protocol) const
{
	ProtocolInfo	proto;

	foreach(proto, mProtocols)
	{
		if (proto.handle == protocol)
		{
			qDebug("proto=%d, name=%s",protocol,proto.name.toAscii().data());
			qDebug("fieldcount = %d", proto.fieldList.size());
			return proto.fieldList.size();
		}
	}

	return 0;
}

int PacketModel::subfieldCount(uint protocol, int field) const
{
	// TODO(MED): Till subfield support is added
	return 0;
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
	registerField(PTYP_L2_802_3_LLC, "Destination Mac", "dstMac");
	registerField(PTYP_L2_802_3_LLC, "Source Mac", "srcMac");
	registerField(PTYP_L2_802_3_LLC, "Destination Service Acces Point", "dsap");
	registerField(PTYP_L2_802_3_LLC, "Source Service Acces Point", "ssap");
	registerField(PTYP_L2_802_3_LLC, "Control", "ctl");

	registerProto(PTYP_L2_SNAP, "802.3 LLC SNAP", "dot3snap");
	registerField(PTYP_L2_SNAP, "Destination Mac", "dstMac");
	registerField(PTYP_L2_SNAP, "Source Mac", "srcMac");
	registerField(PTYP_L2_SNAP, "Destination Service Acces Point", "dsap");
	registerField(PTYP_L2_SNAP, "Source Service Access Point", "ssap");
	registerField(PTYP_L2_SNAP, "Control", "ctl");
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

