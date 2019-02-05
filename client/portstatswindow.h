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

#ifndef _PORT_STATS_WINDOW_H
#define _PORT_STATS_WINDOW_H

#include <QDialog>
#include <QAbstractItemModel>
#include "ui_portstatswindow.h"
#include "portgrouplist.h"
#include "portstatsmodel.h"

class QSortFilterProxyModel;
class QStyledItemDelegate;

class PortStatsWindow : public QWidget, public Ui::PortStatsWindow
{
    Q_OBJECT

public:
    PortStatsWindow(PortGroupList *pgl, QWidget *parent = 0);
    ~PortStatsWindow();

public slots:
    void showMyReservedPortsOnly(bool enabled);

private slots:
    void when_tvPortStats_selectionChanged(const QItemSelection &selected,
                                           const QItemSelection &deselected);

    void on_tbStartTransmit_clicked();
    void on_tbStopTransmit_clicked();

    void on_tbStartCapture_clicked();
    void on_tbStopCapture_clicked();
    void on_tbViewCapture_clicked();

    void on_tbClear_clicked();
    void on_tbClearAll_clicked();
    void on_tbGetStreamStats_clicked();

    void on_tbResolveNeighbors_clicked();
    void on_tbClearNeighbors_clicked();

    void on_tbFilter_clicked();

private:
    PortGroupList           *pgl;
    PortStatsModel          *model;
    QSortFilterProxyModel   *proxyStatsModel;
    QStyledItemDelegate     *statusDelegate;
    QModelIndexList         selectedColumns;

};

#endif

