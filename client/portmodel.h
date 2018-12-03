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

#ifndef _PORT_MODEL_H
#define _PORT_MODEL_H

#include <QAbstractItemModel>
#include <QIcon>

class PortGroupList;
class PortGroup;

class PortModel : public QAbstractItemModel
{
    Q_OBJECT

    friend class PortGroupList;

public:
    PortModel(PortGroupList *p, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, 
            int role = Qt::DisplayRole) const;
    QModelIndex index (int row, int col, 
            const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;    

    bool isPortGroup(const QModelIndex& index);
    bool isPort(const QModelIndex& index);
    quint32 portGroupId(const QModelIndex& index);
    quint32 portId(const QModelIndex& index);

private slots:
    // FIXME: these are invoked from outside - how come they are "private"?
    void when_portGroupDataChanged(int portGroupId, int portId);

    void portGroupAboutToBeAppended();
    void portGroupAppended();
    void portGroupAboutToBeRemoved(PortGroup *portGroup);
    void portGroupRemoved();

    void when_portListChanged();

#if 0
    void triggerLayoutAboutToBeChanged();
    void triggerLayoutChanged();
#endif

private:
    QPixmap statusIcon(int linkState, bool exclusive,
            bool transmit, bool capture) const;

    PortGroupList    *pgl;
};

#endif
