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

#ifndef _DEVICE_GROUP_MODEL_H
#define _DEVICE_GROUP_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class Port;
namespace OstProto {
    class DeviceGroup;
};

class DeviceGroupModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    DeviceGroupModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value,
            int role = Qt::EditRole);
    bool insertRows (int row, int count,
            const QModelIndex &parent = QModelIndex());
    bool removeRows (int row, int count,
            const QModelIndex &parent = QModelIndex());

    void setPort(Port *port);

private:
    int vlanCount(const OstProto::DeviceGroup *deviceGroup) const;

    Port *port_;
};

#endif

