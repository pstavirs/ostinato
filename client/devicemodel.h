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

#ifndef _DEVICE_MODEL_H
#define _DEVICE_MODEL_H

#include <QAbstractTableModel>

class ArpStatusModel;
class NdpStatusModel;
class Port;

class DeviceModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    DeviceModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setPort(Port *port);
    QAbstractItemModel* detailModel(const QModelIndex &index);

public slots:
    void updateDeviceList();

private:
    QVariant drillableStyle(int role) const;

    Port *port_;
    ArpStatusModel *arpStatusModel_;
    NdpStatusModel *ndpStatusModel_;
};

#endif

