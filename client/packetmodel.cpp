#include "packetmodel.h"


PacketModel::PacketModel(Stream *pStream, QObject *parent)
{
	mpStream = pStream;
}

int PacketModel::rowCount(const QModelIndex &parent) const
{
	// Parent - Invisible Root.
	// Children - Top Level Items
	if (!parent.isValid())
	{
		int v = 0;

		if (mpStream->l2.eth.vlanMask & VM_SVLAN_TAGGED)
			v++;
		if (mpStream->l2.eth.vlanMask & VM_CVLAN_TAGGED)
			v++;

		if (mpStream->proto.protoMask & PM_L3_PROTO_NONE)
			return v+2; // L2, Data
		if (mpStream->proto.protoMask & PM_L4_PROTO_NONE)
			return v+3; // L2, L3, Data
		else
			return v+4; // L2, L3, L4, Data
	}

	// Parent - Top Level Item (L2)
	// Children(count) - Second Level Items (L2 fields)
	if (isIndexL2Container(parent))
	{
		switch(mpStream->proto.ft)
		{
			case Stream::e_ft_none:
				return 2; // DstMac, SrcMac
				break;

			case Stream::e_ft_eth_2:
			case Stream::e_ft_802_3_raw:
				return 3; // DstMac, SrcMac, Type/Len
				break;

			case Stream::e_ft_802_3_llc:
			case Stream::e_ft_snap:
				return 5; // DstMac, SrcMac, Type, DSAP, SSAP, CTL, OUI, Type
				break;

			default:
				qDebug("%s: Unsupported frametype", __FUNCTION__);
				return -1;
		}
	}

	// Parent - Top Level Item (SVLAN)
	// Children(count) - Second Level Items (SVLAN fields)
	if (isIndexSvlanContainer(parent))
	{
		return 4;	// TPID, PCP, DE, VlanId
	}

	// Parent - Top Level Item (CVLAN)
	// Children(count) - Second Level Items (CVLAN fields)
	if (isIndexCvlanContainer(parent))
	{
		return 4;	// TPID, Prio, CFI, VlanId
	}

	// Parent - Top Level Item (L3)
	// Children(count) - Second Level Items (L3 fields)
	if (isIndexL3Container(parent))
	{
		// L3 cannot be "None"
		Q_ASSERT(mpStream->proto.protoMask & PM_L3_PROTO_NONE);

		switch(mpStream->proto.etherType)
		{
			case ETH_TYP_IP:
				return 12;	// Ver, HdrLen, TOS, TotLen, Id, Flags, 
							// FragOfs, TTL, Proto, Cksum, SrcIp, DstIp
				break;
			case ETH_TYP_ARP:
				return 0;	// TODO(LOW)
				break;
			default:
				qDebug("%s: Unsupported ethtype", __FUNCTION__);
				return -1;
		}
	}

	// Parent - Top Level Item (L4)
	// Children(count) - Second Level Items (L4 fields)
	if (isIndexL4Container(parent))
	{
		// L4 cannot be "None"
		Q_ASSERT(mpStream->proto.protoMask & PM_L4_PROTO_NONE);

		switch(mpStream->proto.ipProto)
		{
			case IP_PROTO_TCP:
				return 10;	// SrcPort, DstPort, SeqNum, AckNum, HdrLen,
							// Rsvd, Flags, Window, Cksum, UrgPtr, 
				break;
			case IP_PROTO_UDP:
				return 4;	// SrcPort, DstPort, TotLen, Cksum
				break;
			case IP_PROTO_ICMP:
			case IP_PROTO_IGMP:
				return 0;	// TODO(LOW)
				break;
			default:
				qDebug("%s: Unsupported ethtype", __FUNCTION__);
				return -1;
		}
	}

	// Parent - Second Level Item (L2 field)
	// Children(count) - Third Level Items (L2 subfield)
	if (isIndexL2Field(parent))
	{
		return 0;	// No subfields for any L2 field
	}

	// Parent - Second Level Item (L3 field)
	// Children(count) - Third Level Items (L3 subfield)
	if (isIndexL3Field(parent))
	{
		if (isIndexIpField(parent))
			return 0;	// TODO (MED)
		if (isIndexArpField(parent))
			return 0;	// TODO (LOW)

		qDebug("%s: Unknown L3 Field", __FUNCTION__);
		return 0;	// catch all
	}

	// Parent - Second Level Item (L4 field)
	// Children(count) - Third Level Items (L4 subfield)
	if (isIndexL4Field(parent))
	{
		if (isIndexTcpField(parent))
			return 0;	// TODO (MED)
		if (isIndexUdpField(parent))
			return 0;	// No subfields for any UDP fields
		if (isIndexIcmpField(parent))
			return 0;	// TODO (LOW)
		if (isIndexIgmpField(parent))
			return 0;	// TODO (LOW)

		qDebug("%s: Unknown L4 Field", __FUNCTION__);
		return 0;	// catch all
	}

	//qDebug("%s: Catch all - need to investigate", __FUNCTION__);
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
	uint	vlanMask = mpStream->l2.eth.vlanMask;

	if (!hasIndex(row, col, parent))
		goto _exit;

	parentId.w = parent.internalId();

	// Parent - Invisible Root
	// Requested child - First/Top Level Item
	if (parentId.ws.b2 == 0xFF)
	{
		Q_ASSERT(!parent.isValid());

		if (mpStream->l2.eth.vlanMask & VM_UNTAGGED)
			id.ws.b1 = row+2; // Only L2, L3, L4
		else if (VM_SINGLE_TAGGED(vlanMask))
		{
			switch (row)
			{
			case 0: id.ws.b1 = 2; break;	// L2
			case 1: id.ws.b1 = (vlanMask & VM_SVLAN_TAGGED)?0x88:0x81; break;
			case 2: id.ws.b1 = 3; break;	// L3
			case 3: id.ws.b1 = 4; break;	// L4
			case 4: id.ws.b1 = 0; break;	// Data
			default: qWarning("%s: Unexpected row (%d)", __FUNCTION__, row);
			}
		}
		else if (VM_DOUBLE_TAGGED(vlanMask))
		{
			switch (row)
			{
			case 0: id.ws.b1 = 2; break;	// L2
			case 1: id.ws.b1 = 0x88; break;	// SVLAN
			case 2: id.ws.b1 = 0x81; break;	// CVLAN
			case 3: id.ws.b1 = 3; break;	// L3
			case 4: id.ws.b1 = 4; break;	// L4
			case 5: id.ws.b1 = 0; break;	// Data
			default: qWarning("%s: Unexpected row (%d)", __FUNCTION__, row);
			}
		}
		id.ws.b2 = 0xFF;
		index = createIndex(row, col, id.w);
		goto _exit;
	}

	// Parent - First Level Item
	// Requested child - Second Level Item
	if (parentId.ws.b3 == 0xFF)
	{
		Q_ASSERT(parentId.ws.b1 != 0xFF);
		Q_ASSERT(parentId.ws.b2 != 0xFF);

		id.ws.b1 = parentId.ws.b1;
		id.ws.b2 = 0;	// TODO(MED): Set Field Id for subfields
		index = createIndex(row, col, id.w);
		goto _exit;
	}

	// Parent - Second Level Item (Field)
	// Requested child - Third Level Item (Subfield)
	// TODO(MED): Support subfields
	// Till then we return an invalid index

_exit:
	return index;
}

QModelIndex PacketModel::parent(const QModelIndex &index) const
{
	IndexId		id, parentId;

	id.w = index.internalId();
	parentId = id;

	// 1st/Top Level Item - Protocol
	// Requested Parent => Invisible Root
	if (id.ws.b2 == 0xFF)
		return QModelIndex();	

	// Second Level Item - Field
	// Requested Parent => 1st Level Item (Protocol)
	if (id.ws.b3 == 0xFF)
	{
		uint	vlanMask = mpStream->l2.eth.vlanMask;
		int		row = -1;

		parentId.ws.b2 = 0xFF;

		if (vlanMask & VM_UNTAGGED)
		{
			row = parentId.ws.b1 - 2;
		}
		else if (VM_SINGLE_TAGGED(vlanMask))
		{
			switch (parentId.ws.b1)
			{
			case 2: row = 0; break;	// L2
			case 0x88:
			case 0x81: row = 1; break;	// SVlan/CVlan
			case 3: row = 2; break; // L3
			case 4: row = 3; break;	// L4
			case 0: row = 4; break;	// Data
			default: qWarning("%s: Unexpected b1 (%d)", __FUNCTION__, parentId.ws.b1);
			}
		}
		else if (VM_DOUBLE_TAGGED(vlanMask))
		{
			switch (parentId.ws.b1)
			{
			case 2: row = 0; break;	// L2
			case 0x88: row = 1; break; // Svlan
			case 0x81: row = 2; break;	// CVlan
			case 3: row = 3; break; // L3
			case 4: row = 4; break;	// L4
			case 0: row = 5; break;	// Data
			default: qWarning("%s: Unexpected b1 (%d)", __FUNCTION__, parentId.ws.b1);
			}
		}
		else
			qWarning("%s: Unhandled leg", __FUNCTION__);

		return createIndex(row, 0, parentId.w);	
	}

	// Third Level Item - Subfield
	// Requested Parent => 2nd Level Item (Field)
	// TODO(Med)
	qWarning("%s: Unexpected leg", __FUNCTION__);
	return QModelIndex();
}

QVariant PacketModel::data(const QModelIndex &index, int role) const
{
	IndexId	id;

	id.w = index.internalId();

	if (id.ws.b2 == 0xFF)
		return QString("Protocol Header");
	else
		return QString("Field: Value");
}


/*
** --------------- Private Stuff -----------------
*/
typedef union
{
	quint32	w;
	struct
	{
		quint8	b1;
		quint8	b2;
		quint8	b3;
		quint8	b4;
	} ws;
} IndexId;

bool PacketModel::isIndexContainer(const QModelIndex& index, int level) const
{
	IndexId	id;
	
	id.w = index.internalId();
	if ((id.ws.b1 == level) && (id.ws.b2 == 0xFF))
		return true;
	else 
		return false;
}

bool PacketModel::isIndexL2Container(const QModelIndex& index) const
{
	return isIndexContainer(index, 2);
}

bool PacketModel::isIndexSvlanContainer(const QModelIndex& index) const
{
	return isIndexContainer(index, 0x88);
}

bool PacketModel::isIndexCvlanContainer(const QModelIndex& index) const
{
	return isIndexContainer(index, 0x81);
}

bool PacketModel::isIndexL3Container(const QModelIndex& index) const
{
	return isIndexContainer(index, 3);
}

bool PacketModel::isIndexL4Container(const QModelIndex& index) const
{
	return isIndexContainer(index, 4);
}

bool PacketModel::isIndexField(const QModelIndex& index, int level) const
{
	IndexId	id;
	
	id.w = index.internalId();
	if ((id.ws.b1 == level) && (id.ws.b2 != 0xFF) && (id.ws.b3 == 0xFF))
		return true;
	else 
		return false;
}

bool PacketModel::isIndexL2Field(const QModelIndex& index) const
{
	return isIndexField(index, 2);
}

bool PacketModel::isIndexL3Field(const QModelIndex& index) const
{
	return isIndexField(index, 3);
}

bool PacketModel::isIndexL4Field(const QModelIndex& index) const
{
	return isIndexField(index, 4);
}

bool PacketModel::isIndexIpField(const QModelIndex& index) const
{
	IndexId	id;
	
	id.w = index.internalId();
	if ((id.ws.b1 == 3) && (id.ws.b2 == 1) && (id.ws.b3 == 0xFF))
		return true;
	else 
		return false;
}

bool PacketModel::isIndexArpField(const QModelIndex& index) const
{
	IndexId	id;
	
	id.w = index.internalId();
	if ((id.ws.b1 == 3) && (id.ws.b2 == 2) && (id.ws.b3 == 0xFF))
		return true;
	else 
		return false;
}

bool PacketModel::isIndexL4ProtoField(const QModelIndex& index, int proto) const
{
	IndexId	id;
	
	id.w = index.internalId();
	if ((id.ws.b1 == 4) && (id.ws.b2 == proto) && (id.ws.b3 == 0xFF))
		return true;
	else 
		return false;
}

bool PacketModel::isIndexTcpField(const QModelIndex& index) const
{
	return isIndexL4ProtoField(index, IP_PROTO_TCP);
}

bool PacketModel::isIndexUdpField(const QModelIndex& index) const
{
	return isIndexL4ProtoField(index, IP_PROTO_UDP);
}

bool PacketModel::isIndexIcmpField(const QModelIndex& index) const
{
	return isIndexL4ProtoField(index, IP_PROTO_ICMP);
}

bool PacketModel::isIndexIgmpField(const QModelIndex& index) const
{
	return isIndexL4ProtoField(index, IP_PROTO_IGMP);
}
