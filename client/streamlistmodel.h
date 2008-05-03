#ifndef _STREAM_LIST_MODEL_H
#define _STREAM_LIST_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>

static QStringList StreamListCols = (QStringList()
	<< "Icon"
	<< "Name"
	<< "Enable"
);

#define MAX_ROWS	5
#define MAX_COLS	3

class StreamListModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		//StreamListModel(QObject *parent = 0) : QAbstractTableModel(parent) {}
		StreamListModel(QObject *parent = 0);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		QVariant data(const QModelIndex &index, int role) const;
		bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		// QModelIndex index (int portNum, PortStat stat, const QModelIndex & parent = QModelIndex() ) const;

	public slots:
//		void on_portStatsUpdate(int port, void*stats);

	private:
		// FIXME: temp
//#define NUM_PORTS		2
//		int dummyStats[NUM_PORTS][e_STAT_MAX];
		struct {
			QString		streamName;
			bool		isEnabled;
		} streamList[MAX_ROWS];

};

#endif
