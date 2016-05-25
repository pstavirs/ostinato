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

#ifndef _PORT_GROUP_LIST_H
#define _PORT_GROUP_LIST_H

#include "devicegroupmodel.h"
#include "devicemodel.h"
#include "portgroup.h"
#include "portmodel.h"
#include "portstatsmodel.h"
#include "streammodel.h"

class PortModel;
class StreamModel;

class PortGroupList : public QObject {

    Q_OBJECT

    friend class PortModel;
    friend class StreamModel;
    friend class PortStatsModel;

    QList<PortGroup*>    mPortGroups;    
    PortModel            mPortGroupListModel;
    StreamModel          mStreamListModel;
    PortStatsModel       mPortStatsModel;
    DeviceGroupModel     mDeviceGroupModel;
    DeviceModel          mDeviceModel;

    QObject *streamModelTester_;
    QObject *portModelTester_;
    QObject *portStatsModelTester_;
    QObject *deviceGroupModelTester_;
    QObject *deviceModelTester_;

// Methods
public:
    PortGroupList();
    ~PortGroupList();

    PortModel* getPortModel() { return &mPortGroupListModel; }
    PortStatsModel* getPortStatsModel() { return &mPortStatsModel; }
    StreamModel* getStreamModel() { return &mStreamListModel; }
    DeviceGroupModel* getDeviceGroupModel() { return &mDeviceGroupModel; }
    DeviceModel* getDeviceModel() { return &mDeviceModel; }

    bool isPortGroup(const QModelIndex& index);
    bool isPort(const QModelIndex& index);
    PortGroup& portGroup(const QModelIndex& index);
    Port& port(const QModelIndex& index);

    int numPortGroups() { return mPortGroups.size(); }
    PortGroup& portGroupByIndex(int index) { return *(mPortGroups[index]); }

    void addPortGroup(PortGroup &portGroup);
    void removePortGroup(PortGroup &portGroup);
    void removeAllPortGroups();

private:
    int indexOfPortGroup(quint32 portGroupId);

};

#endif
