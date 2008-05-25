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
	QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent(const QModelIndex &index) const;		

private:
	Stream	*mpStream;
	typedef union _IndexId
	{
		quint32	w;
		struct
		{
			quint8	b1;		// 1st Level
			quint8	b2;		// 2nd Level
			quint8	b3;		// 3rd Level
			quint8	b4;		// Reserved
		} ws;
	} IndexId;

	bool PacketModel::isIndexContainer(const QModelIndex& index, int level) const;
	bool PacketModel::isIndexL2Container(const QModelIndex& index) const;
	bool PacketModel::isIndexSvlanContainer(const QModelIndex& index) const;
	bool PacketModel::isIndexCvlanContainer(const QModelIndex& index) const;
	bool PacketModel::isIndexL3Container(const QModelIndex& index) const;
	bool PacketModel::isIndexL4Container(const QModelIndex& index) const;
	bool PacketModel::isIndexField(const QModelIndex& index, int level) const;
	bool PacketModel::isIndexL2Field(const QModelIndex& index) const;
	bool PacketModel::isIndexL3Field(const QModelIndex& index) const;
	bool PacketModel::isIndexL4Field(const QModelIndex& index) const;
	bool PacketModel::isIndexIpField(const QModelIndex& index) const;
	bool PacketModel::isIndexArpField(const QModelIndex& index) const;
	bool PacketModel::isIndexL4ProtoField(const QModelIndex& index, int proto) const;
	bool PacketModel::isIndexTcpField(const QModelIndex& index) const;
	bool PacketModel::isIndexUdpField(const QModelIndex& index) const;
	bool PacketModel::isIndexIcmpField(const QModelIndex& index) const;
	bool PacketModel::isIndexIgmpField(const QModelIndex& index) const;
};
#endif

