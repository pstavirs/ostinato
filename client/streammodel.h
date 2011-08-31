/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

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
        StreamStatus,
        StreamName,
        StreamNextWhat,

        StreamMaxFields
    };

    static QStringList    nextWhatOptionList()
        { return QStringList() << "Stop" << "Next" << "Goto first"; }

    public slots:
        void setCurrentPortIndex(const QModelIndex &current);

    private slots:
        void when_mCurrentPort_streamListChanged(int portGroupId, int portId);
};

#endif
