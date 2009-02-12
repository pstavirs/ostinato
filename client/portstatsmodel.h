#ifndef _PORT_STATS_MODEL_H
#define _PORT_STATS_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>

typedef enum {
	e_STAT_FRAMES_RCVD = 0,
	e_STAT_FRAMES_SENT,
	e_STAT_FRAME_SEND_RATE,
	e_STAT_FRAME_RECV_RATE,
	e_STAT_BYTES_RCVD,
	e_STAT_BYTES_SENT,
	e_STAT_BYTE_SEND_RATE,
	e_STAT_BYTE_RECV_RATE,
#if 0
	e_STAT_FRAMES_RCVD_NIC,
	e_STAT_FRAMES_SENT_NIC,
	e_STAT_BYTES_RCVD_NIC,
	e_STAT_BYTES_SENT_NIC,
#endif
	e_STAT_MAX
} PortStat;

static QStringList PortStatName = (QStringList()
	<< "Frames Received"
	<< "Frames Sent"
	<< "Frame Send Rate (fps)"
	<< "Frame Receive Rate (fps)"
	<< "Bytes Received"
	<< "Bytes Sent"
	<< "Byte Send Rate (Bps)"
	<< "Byte Receive Rate (Bps)"
#if 0
	<< "Frames Received (NIC)"
	<< "Frames Sent (NIC)"
	<< "Bytes Received (NIC)"
	<< "Bytes Sent (NIC)"
#endif
);

class PortGroupList;

class PortStatsModel : public QAbstractTableModel
{
	Q_OBJECT

	public:

		PortStatsModel(PortGroupList *p, QObject *parent = 0);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, 
			int role = Qt::DisplayRole) const;

		class PortGroupAndPortList {
			public:
			uint portGroupId;
			QList<uint> portList;
		};
		void portListFromIndex(QModelIndexList indices, 
				QList<PortGroupAndPortList> &portList);

	public slots:
		void when_portListChanged();
		void on_portStatsUpdate(int port, void*stats);
		void when_portGroup_stats_update(quint32 portGroupId);

	private slots:
		void updateStats();

	private:
		PortGroupList	*pgl;

		// numPorts stores the num of ports per portgroup
		// in the same order as the portgroups are index in the pgl
		// Also it stores them as cumulative totals
		QList<quint16>	numPorts;

		void getDomainIndexes(const QModelIndex &index,
			  uint &portGroupIdx, uint &portIdx) const;

};

#endif
