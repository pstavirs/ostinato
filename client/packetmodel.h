#ifndef _PACKET_MODEL_H
#define _PACKET_MODEL_H

#include <QAbstractItemModel>
#include "stream.h"

#define NEW_IMPL			// FIXME(HI) - Use this and remove old one

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
			quint16	type;
#define ITYP_PROTOCOL	1
#define ITYP_FIELD		2
			quint16	protocol;	// protocol is valid for both ITYPs
		} ws;
	} IndexId;

	Stream	*mpStream;

#ifdef NEW_IMPL
// Nothing - required stuff is part of Stream Class
#else
	QList<uint> mPacketProtocols;

	typedef struct
	{
		QString			name;
		QString			abbr;
		QString			textValue;
	} FieldInfo;

	typedef struct
	{
		uint				handle;
		QString				name;
		QString				abbr;
		QList<FieldInfo>	fieldList;
	} ProtocolInfo;

	//! Contains registration info (name, size etc) for all protocols
	// and fields within the protocol
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

	int protoCount() const;
	int fieldCount(int protocol) const;
	QString protoName(int protocol) const;
	QString fieldName(int protocol, int field) const;
	QVariant fieldTextValue(int protocol, int field) const;

	QVariant ethField(int field, int role) const;
	QVariant llcField(int field, int role) const;
	QVariant snapField(int field, int role) const;
	QVariant svlanField(int field, int role) const;
	QVariant ipField(int field, int role) const;
	QVariant tcpField(int field, int role) const;
	QVariant udpField(int field, int role) const;

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

#endif // NEW_IMPL

#define FROL_NAME			1
#define FROL_TEXT_VALUE		2

#define BASE_HEX			16
};
#endif

