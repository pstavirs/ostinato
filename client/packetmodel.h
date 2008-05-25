#ifndef _PACKET_MODEL_H
#define _PACKET_MODEL_H

#include <QAbstractItemModel>
#include "stream.h"

class PacketModel: public QAbstractItemModel
{

public:
	PacketModel(Stream *pStream, QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, 
		int role = Qt::DisplayRole) const { return QVariant(); } ;
	QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent(const QModelIndex &index) const;		

private:
	typedef union _IndexId
	{
		quint32	w;
		struct
		{
			quint8	type;
#define ITYP_PROTOCOL	1
#define ITYP_FIELD		2
#define ITYP_SUBFIELD	3
			quint8	protocol;
			quint8	field;
			quint8	subfield;	
		} ws;
	} IndexId;

	typedef struct
	{
		QString			name;
		QString			abbr;
	} FieldInfo;

	typedef struct
	{
		uint				handle;
		QString				name;
		QString				abbr;
		QList<FieldInfo>	fieldList;
	} ProtocolInfo;

	Stream	*mpStream;
	QList<uint>	mPacketProtocols;
	QList<ProtocolInfo> mProtocols;

	void registerProto(uint handle, char *name, char *abbr);
	void registerField(uint protoHandle, char *name, char *abbr);

	void registerFrameTypeProto();
	void registerVlanProto();
	void registerIpProto();
	void registerArpProto();
	void registerTcpProto();
	void registerUdpProto();
	void registerIcmpProto();
	void registerIgmpProto();
	void registerData();
	void registerInvalidProto();

	void populatePacketProtocols();
	int fieldCount(uint protocol) const;
	int subfieldCount(uint protocol, int field) const;

// FIXME(HIGH): Is this how I want this?
#define PTYP_L2_NONE		1
#define PTYP_L2_ETH_2		2
#define PTYP_L2_802_3_RAW	3
#define PTYP_L2_802_3_LLC	4
#define PTYP_L2_SNAP		5

#define PTYP_SVLAN			10
#define PTYP_CVLAN			11

#define PTYP_L3_IP			30
#define PTYP_L3_ARP			31

#define PTYP_L4_TCP			40	
#define PTYP_L4_UDP			41	
#define PTYP_L4_ICMP		42
#define PTYP_L4_IGMP		43

#define PTYP_INVALID		0
#define PTYP_DATA			0xFF

};
#endif

