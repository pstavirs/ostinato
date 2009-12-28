#ifndef _STREAM_MODEL_H
#define _STREAM_MODEL_H

#include <QStringList>
#include <QAbstractTableModel>
#include "port.h"

class PortGroupList;

class StreamModel : public QAbstractTableModel
{
    Q_OBJECT

    Port            *mCurrentPort;
    PortGroupList    *pgl;

    public:
        StreamModel(PortGroupList *p, QObject *parent = 0);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value,
            int role = Qt::EditRole);
        QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;
        bool insertRows (int row, int count,
            const QModelIndex & parent = QModelIndex());
        bool removeRows (int row, int count,
            const QModelIndex & parent = QModelIndex());
    
#if 0 // CleanedUp!
        // FIXME(HIGH): This *is* like a kludge
        QList<Stream>* currentPortStreamList() 
            { return &mCurrentPort->mStreams; }
#endif

    public:
    enum StreamFields {
        StreamIcon = 0,
        StreamName,
        StreamStatus,
        StreamNextWhat,

        StreamMaxFields
    };

    static QStringList    nextWhatOptionList()
        { return QStringList() << "Stop" << "Next" << "Goto first"; }

    public slots:
        void setCurrentPortIndex(const QModelIndex &current);

};

#endif
