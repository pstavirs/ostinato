/*
Copyright (C) 2016 Srivats P.

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

#ifndef _ARP_STATUS_MODEL_H
#define _ARP_STATUS_MODEL_H

#include <QAbstractTableModel>

class Port;
namespace OstEmul {
    class DeviceNeighborList;
}

class ArpStatusModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    ArpStatusModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setDeviceIndex(Port *port, int deviceIndex);

public slots:
    void updateArpStatus();

private:
    Port *port_;
    int deviceIndex_;
    const OstEmul::DeviceNeighborList *neighbors_;
};

#endif

