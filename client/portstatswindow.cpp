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
#include "portstatsmodel.h"
#include "portstatsfilterdialog.h"

#include "QHeaderView"

PortStatsWindow::PortStatsWindow(PortGroupList *pgl, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    this->pgl = pgl;
    model = pgl->getPortStatsModel();
    tvPortStats->setModel(model);

    tvPortStats->verticalHeader()->setHighlightSections(false);
    tvPortStats->verticalHeader()->setDefaultSectionSize(
        tvPortStats->verticalHeader()->minimumSectionSize());

}

PortStatsWindow::~PortStatsWindow()
{
}

/* ------------- SLOTS -------------- */

void PortStatsWindow::on_tbStartTransmit_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    pgpl;

    // Get selected ports
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               pgpl);

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
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               pgpl);

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
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               pgpl);

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
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               pgpl);

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
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               pgpl);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < pgpl.size(); i++)
    {
        pgl->portGroupByIndex(pgpl.at(i).portGroupId).
            viewCapture(&pgpl[i].portList);
    }
}

void PortStatsWindow::on_tbClear_clicked()
{
    QList<PortStatsModel::PortGroupAndPortList>    portList;

    // Get selected ports
    model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
               portList);

    // Clear selected ports, portgroup by portgroup
    for (int i = 0; i < portList.size(); i++)
    {
        pgl->portGroupByIndex(portList.at(i).portGroupId).
            clearPortStats(&portList[i].portList);
    }
}

void PortStatsWindow::on_tbClearAll_clicked()
{
    for (int i = 0; i < pgl->numPortGroups(); i++)
    {
        pgl->portGroupByIndex(0).clearPortStats();
    }
}

void PortStatsWindow::on_tbFilter_clicked()
{
    bool ok;
    QList<uint>    currentColumns, newColumns;
    PortStatsFilterDialog    dialog;

    for(int i = 0; i < model->columnCount(); i++)
        if (!tvPortStats->isColumnHidden(i))
            currentColumns.append(i);

    newColumns = dialog.getItemList(&ok, model, Qt::Horizontal, currentColumns);

    if (ok)
    {
        // hide/show sections first ...
        for(int i = 0; i < model->columnCount(); i++)
            tvPortStats->setColumnHidden(i, !newColumns.contains(i));

        // ... then for the 'shown' columns, set the visual index
        for(int i = 0; i < newColumns.size(); i++)
        {
            tvPortStats->horizontalHeader()->moveSection(tvPortStats->
                    horizontalHeader()->visualIndex(newColumns.at(i)), i);
        }
    }
}
