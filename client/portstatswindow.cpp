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


#include "portstatswindow.h"

#include "portstatsfilterdialog.h"
#include "portstatsmodel.h"
#include "portstatsproxymodel.h"
#include "settings.h"

#include <QHeaderView>

PortStatsWindow::PortStatsWindow(PortGroupList *pgl, QWidget *parent)
    : QWidget(parent), proxyStatsModel(NULL)
{
    setupUi(this);

    this->pgl = pgl;
    model = pgl->getPortStatsModel();

    proxyStatsModel = new PortStatsProxyModel(this);
    if (proxyStatsModel) {
        proxyStatsModel->setSourceModel(model);
        tvPortStats->setModel(proxyStatsModel);
    }
    else
        tvPortStats->setModel(model);

    tvPortStats->verticalHeader()->setHighlightSections(false);
    tvPortStats->verticalHeader()->setDefaultSectionSize(
        tvPortStats->verticalHeader()->minimumSectionSize());

    connect(tvPortStats->selectionModel(),
            SIGNAL(selectionChanged(
                    const QItemSelection&, const QItemSelection&)),
            SLOT(when_tvPortStats_selectionChanged(
                    const QItemSelection&, const QItemSelection&)));
    when_tvPortStats_selectionChanged(QItemSelection(), QItemSelection());
}

PortStatsWindow::~PortStatsWindow()
{
    delete proxyStatsModel;
}

/* ------------- SLOTS (public) -------------- */

void PortStatsWindow::showMyReservedPortsOnly(bool enabled)
{
    if (!proxyStatsModel)
        return;

    if (enabled) {
        QString rx(appSettings->value(kUserKey, kUserDefaultValue).toString());
        proxyStatsModel->setFilterRegExp(QRegExp::escape(rx));
    }
    else
        proxyStatsModel->setFilterRegExp(QRegExp(".*")); // match all
}

/* ------------- SLOTS (private) -------------- */

void PortStatsWindow::when_tvPortStats_selectionChanged(
        const QItemSelection& /*selected*/,
        const QItemSelection& /*deselected*/)
{
    QModelIndexList indexList = 
            tvPortStats->selectionModel()->selectedColumns();

    if (proxyStatsModel) {
        selectedColumns.clear();
        foreach(QModelIndex index, indexList)
            selectedColumns.append(proxyStatsModel->mapToSource(index));
    }
    else 
        selectedColumns = indexList;

    bool isEmpty = selectedColumns.isEmpty();

    tbStartTransmit->setDisabled(isEmpty);
    tbStopTransmit->setDisabled(isEmpty);

    tbStartCapture->setDisabled(isEmpty);
    tbStopCapture->setDisabled(isEmpty);
    tbViewCapture->setDisabled(isEmpty);

    tbClear->setDisabled(isEmpty);

    tbResolveNeighbors->setDisabled(isEmpty);
    tbClearNeighbors->setDisabled(isEmpty);
}

void PortStatsWindow::on_tbStartTransmit_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(selectedColumns, pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            startTx(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbStopTransmit_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(selectedColumns, pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            stopTx(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbStartCapture_clicked()
{
    // TODO(MED)
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(selectedColumns, pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            startCapture(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbStopCapture_clicked()
{
    // TODO(MED)
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(selectedColumns, pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            stopCapture(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbViewCapture_clicked()
{
    // TODO(MED)
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(selectedColumns, pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            viewCapture(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbResolveNeighbors_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    portList;

    // Get selected ports
    model->portListFromIndex(selectedColumns, portList);

    // Resolve ARP/ND for selected ports, portgroup by portgroup
    for (int i = 0; i < portList.size(); i++)
    {
        pgl->portGroupByIndex(portList.at(i).portGroupId).
            resolveDeviceNeighbors(&portList[i].portList);
    }
}

void PortStatsWindow::on_tbClearNeighbors_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    portList;

    // Get selected ports
    model->portListFromIndex(selectedColumns, portList);

    // Clear ARP/ND for ports, portgroup by portgroup
    for (int i = 0; i < portList.size(); i++)
    {
        pgl->portGroupByIndex(portList.at(i).portGroupId).
            clearDeviceNeighbors(&portList[i].portList);
    }
}

void PortStatsWindow::on_tbClear_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    portList;

    // Get selected ports
    model->portListFromIndex(selectedColumns, portList);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < portList.size(); i++)
    {
        pgl->portGroupByIndex(portList.at(i).portGroupId).
            clearPortStats(&portList[i].portList);
    }
}

// 'All' => all ports currently visible, not all ports in all portgroups
void PortStatsWindow::on_tbClearAll_clicked()
{
    QAbstractItemModel *mdl = tvPortStats->model();
    QModelIndexList shownColumns;
    QList<PortStatsModel::PortGroupAndPortList> portList;

    // Find the 'visible' columns
    for(int vi = 0; vi < mdl->columnCount(); vi++) {
        int li = tvPortStats->horizontalHeader()->logicalIndex(vi);
        if (!tvPortStats->isColumnHidden(li)) {
            shownColumns.append(mdl->index(0, li));
        }
    }

    // Get ports corresponding to the shown columns
    model->portListFromIndex(shownColumns, portList);

    // Clear shown ports, portgroup by portgroup
    for (int i = 0; i < portList.size(); i++)
    {
        pgl->portGroupByIndex(portList.at(i).portGroupId)
                    .clearPortStats(&portList[i].portList);
    }
}

void PortStatsWindow::on_tbFilter_clicked()
{
    bool ok;
    QList<uint>    currentColumns, newColumns;
    PortStatsFilterDialog    dialog;
    QAbstractItemModel *mdl = tvPortStats->model();

    // create the input list for the filter dialog -
    // list of logical-indexes ordered by their current visual indexes
    for(int vi = 0; vi < mdl->columnCount(); vi++) {
        int li = tvPortStats->horizontalHeader()->logicalIndex(vi);
        if (!tvPortStats->isColumnHidden(li)) {
            currentColumns.append(li);
        }
    }

    // return list from the filter dialog -
    // list of logical-indexes ordered by their new visual indexes
    newColumns = dialog.getItemList(&ok, mdl, Qt::Horizontal, currentColumns);

    if (ok)
    {
        QHeaderView *hv = tvPortStats->horizontalHeader();

        // hide/show sections first ...
        for(int li = 0; li < mdl->columnCount(); li++)
            tvPortStats->setColumnHidden(li, !newColumns.contains(li));

        // ... then for the 'shown' columns, set the visual index
        for(int vi = 0; vi < newColumns.size(); vi++)
            hv->moveSection(hv->visualIndex(newColumns.at(vi)), vi);
    }
}
