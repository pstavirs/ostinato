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

#ifndef _PORT_STATS_FILTER_DIALOG_H
#define _PORT_STATS_FILTER_DIALOG_H

#include <QDialog>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include "ui_portstatsfilter.h"
#include "portgrouplist.h"

class PortStatsFilterDialog : public QDialog, public Ui::PortStatsFilterDialog
{
    Q_OBJECT

public:
    PortStatsFilterDialog(QWidget *parent = 0);
    QList<uint> getItemList(bool* ok, QAbstractItemModel *model,
        Qt::Orientation orientation = Qt::Vertical, 
        QList<uint> initial = QList<uint>());

private:
    enum ItemRole {
        kLogicalIndex = Qt::UserRole + 1,
        kVisualIndex
    };
    QStandardItemModel    mUnselected;
    QStandardItemModel    mSelected;

private slots:
    void on_tbSelectIn_clicked();
    void on_tbSelectOut_clicked();
    void on_lvUnselected_doubleClicked(const QModelIndex &index);
    void on_lvSelected_doubleClicked(const QModelIndex &index);
};

#endif

