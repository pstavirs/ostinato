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

#ifndef _PACKET_MODEL_H
#define _PACKET_MODEL_H

#include <QAbstractItemModel>

class ProtocolListIterator;
class AbstractProtocol;

class PacketModel: public QAbstractItemModel
{

public:
    PacketModel(QObject *parent = 0);
    void setSelectedProtocols(ProtocolListIterator &iter);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int /*section*/, Qt::Orientation /*orientation*/, 
        int /*role= Qt::DisplayRole*/) const { 
        return QVariant(); 
    }
    QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent(const QModelIndex &index) const;        

private:
    typedef union _IndexId
    {
        quint32    w;
        struct
        {
            quint16    type;
#define ITYP_PROTOCOL   1
#define ITYP_FIELD      2
            quint16    protocol;    // protocol is valid for both ITYPs
        } ws;
    } IndexId;

    QList<const AbstractProtocol*> mSelectedProtocols;
};
#endif

