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
along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#include "deviceswidget.h"

#include "devicegroupdialog.h"
#include "port.h"
#include "portgrouplist.h"

#include <QHeaderView>
#include <QKeyEvent>

DevicesWidget::DevicesWidget(QWidget *parent)
    : QWidget(parent), portGroups_(NULL)
{
    setupUi(this);
    deviceGroupList->setVisible(deviceConfig->isChecked());
    deviceList->setVisible(deviceInfo->isChecked());
    setDeviceInfoButtonsVisible(deviceInfo->isChecked());
    deviceDetail->hide();

    deviceGroupList->verticalHeader()->setDefaultSectionSize(
            deviceGroupList->verticalHeader()->minimumSectionSize());
    deviceList->verticalHeader()->setDefaultSectionSize(
            deviceList->verticalHeader()->minimumSectionSize());
    deviceDetail->verticalHeader()->setDefaultSectionSize(
            deviceDetail->verticalHeader()->minimumSectionSize());

    // Populate DeviceGroup Context Menu Actions
    deviceGroupList->addAction(actionNewDeviceGroup);
    deviceGroupList->addAction(actionEditDeviceGroup);
    deviceGroupList->addAction(actionDeleteDeviceGroup);

    // DevicesWidget's actions is an aggegate of all sub-widget's actions
    addActions(deviceGroupList->actions());
}

void DevicesWidget::setPortGroupList(PortGroupList *portGroups)
{
    portGroups_ = portGroups;

    deviceGroupList->setModel(portGroups_->getDeviceGroupModel());
    deviceList->setModel(portGroups_->getDeviceModel());

    connect(portGroups_->getDeviceGroupModel(),
        SIGNAL(rowsInserted(QModelIndex, int, int)),
        SLOT(updateDeviceViewActions()));
    connect(portGroups_->getDeviceGroupModel(),
        SIGNAL(rowsRemoved(QModelIndex, int, int)),
        SLOT(updateDeviceViewActions()));

    connect(deviceGroupList->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        SLOT(updateDeviceViewActions()));
    connect(deviceGroupList->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        SLOT(updateDeviceViewActions()));

    connect(deviceList->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        SLOT(when_deviceList_currentChanged(const QModelIndex&)));

    // FIXME: hardcoding
    deviceGroupList->resizeColumnToContents(1); // Vlan Count
    deviceGroupList->resizeColumnToContents(2); // Device Count

    // FIXME: hardcoding
    deviceList->resizeColumnToContents(1); // Vlan Id(s)
    deviceList->resizeColumnToContents(6); // ARP Info
    deviceList->resizeColumnToContents(7); // NDP Info

    updateDeviceViewActions();
}

void DevicesWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        deviceDetail->hide();
        event->accept();
    }
    else
        event->ignore();
}

void DevicesWidget::setCurrentPortIndex(const QModelIndex &portIndex)
{
    Port *port = NULL;

    if (!portGroups_)
       return;

    // XXX: We assume portIndex corresponds to sourceModel, not proxyModel
    if (portGroups_->isPort(portIndex))
        port = &portGroups_->port(portIndex);

    portGroups_->getDeviceGroupModel()->setPort(port);
    portGroups_->getDeviceModel()->setPort(port);
    currentPortIndex_ = portIndex;

    updateDeviceViewActions();
}

void DevicesWidget::updateDeviceViewActions()
{
    QItemSelectionModel *devSel = deviceGroupList->selectionModel();

    if (!portGroups_)
       return;

    // For some reason hasSelection() returns true even if selection size is 0
    // so additional check for size introduced
    if (devSel->hasSelection() && (devSel->selection().size() > 0)) {
        // If more than one non-contiguous ranges selected,
        // disable "New" and "Edit"
        if (devSel->selection().size() > 1) {
            actionNewDeviceGroup->setDisabled(true);
            actionEditDeviceGroup->setDisabled(true);
        }
        else {
            actionNewDeviceGroup->setEnabled(true);

            // Enable "Edit" only if the single range has a single row
            if (devSel->selection().at(0).height() > 1)
                actionEditDeviceGroup->setDisabled(true);
            else
                actionEditDeviceGroup->setEnabled(true);
        }

        // Delete is always enabled as long as we have a selection
        actionDeleteDeviceGroup->setEnabled(true);
    }
    else {
        qDebug("No device selection");
        if (portGroups_->isPort(currentPortIndex_))
            actionNewDeviceGroup->setEnabled(true);
        else
            actionNewDeviceGroup->setDisabled(true);
        actionEditDeviceGroup->setDisabled(true);
        actionDeleteDeviceGroup->setDisabled(true);
    }
}

//
// DeviceGroup slots
//

void DevicesWidget::on_deviceInfo_toggled(bool checked)
{
    setDeviceInfoButtonsVisible(checked);
    deviceGroupList->setHidden(checked);
    deviceList->setVisible(checked);
    deviceDetail->hide();
}

void DevicesWidget::on_actionNewDeviceGroup_triggered()
{
    if (!portGroups_)
       return;

    // In case nothing is selected, insert 1 row at the end
    int row = portGroups_->getDeviceGroupModel()->rowCount(), count = 1;
    QItemSelection selection = deviceGroupList->selectionModel()->selection();

    // In case we have a single range selected; insert as many rows as
    // in the singe selected range before the top of the selected range
    if (selection.size() == 1) {
        row = selection.at(0).top();
        count = selection.at(0).height();
    }

    portGroups_->getDeviceGroupModel()->insertRows(row, count);
}

void DevicesWidget::on_actionDeleteDeviceGroup_triggered()
{
    QModelIndex index;

    if (!portGroups_)
       return;

    if (deviceGroupList->selectionModel()->hasSelection()) {
        while(deviceGroupList->selectionModel()->selectedRows().size()) {
            index = deviceGroupList->selectionModel()->selectedRows().at(0);
            portGroups_->getDeviceGroupModel()->removeRows(index.row(), 1);
        }
    }
}

void DevicesWidget::on_actionEditDeviceGroup_triggered()
{
    QItemSelection selection = deviceGroupList->selectionModel()->selection();

    // Ensure we have only one range selected which contains only one row
    if ((selection.size() == 1) && (selection.at(0).height() == 1))
        on_deviceGroupList_activated(selection.at(0).topLeft());
}

void DevicesWidget::on_deviceGroupList_activated(const QModelIndex &index)
{
    if (!index.isValid() || !portGroups_)
        return;

    DeviceGroupDialog dgd(&portGroups_->port(currentPortIndex_), index.row());
    dgd.exec();
}

void DevicesWidget::on_refresh_clicked()
{
    if (!portGroups_)
       return;

    Q_ASSERT(portGroups_->isPort(currentPortIndex_));
    QModelIndex curPortGroup = portGroups_->getPortModel()
                                                ->parent(currentPortIndex_);
    Q_ASSERT(curPortGroup.isValid());
    Q_ASSERT(portGroups_->isPortGroup(curPortGroup));

    deviceDetail->hide();
    portGroups_->portGroup(curPortGroup)
                    .getDeviceInfo(portGroups_->port(currentPortIndex_).id());
}

void DevicesWidget::on_resolveNeighbors_clicked()
{
    if (!portGroups_)
       return;

    Q_ASSERT(portGroups_->isPort(currentPortIndex_));
    QModelIndex curPortGroup = portGroups_->getPortModel()
                                                ->parent(currentPortIndex_);
    Q_ASSERT(curPortGroup.isValid());
    Q_ASSERT(portGroups_->isPortGroup(curPortGroup));

    deviceDetail->hide();
    QList<uint> portList({portGroups_->port(currentPortIndex_).id()});
    portGroups_->portGroup(curPortGroup).resolveDeviceNeighbors(&portList);
    portGroups_->portGroup(curPortGroup).getDeviceInfo(portList.at(0));
}

void DevicesWidget::on_clearNeighbors_clicked()
{
    if (!portGroups_)
       return;

    Q_ASSERT(portGroups_->isPort(currentPortIndex_));
    QModelIndex curPortGroup = portGroups_->getPortModel()
                                                ->parent(currentPortIndex_);
    Q_ASSERT(curPortGroup.isValid());
    Q_ASSERT(portGroups_->isPortGroup(curPortGroup));

    deviceDetail->hide();
    QList<uint> portList({portGroups_->port(currentPortIndex_).id()});
    portGroups_->portGroup(curPortGroup).clearDeviceNeighbors(&portList);
    portGroups_->portGroup(curPortGroup).getDeviceInfo(portList.at(0));
}

void DevicesWidget::when_deviceList_currentChanged(const QModelIndex &index)
{
    if (!index.isValid() || !portGroups_)
        return;

    QAbstractItemModel *detailModel = portGroups_->getDeviceModel()
                                                        ->detailModel(index);

    deviceDetail->setModel(detailModel);
    deviceDetail->setVisible(detailModel != NULL);
}

void DevicesWidget::setDeviceInfoButtonsVisible(bool show)
{
    refresh->setVisible(show);
    arpNdpLabel->setVisible(show);
    resolveNeighbors->setVisible(show);
    clearNeighbors->setVisible(show);
}
