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

#include "portstatsfilterdialog.h"

PortStatsFilterDialog::PortStatsFilterDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    mUnselected.setSortRole(kLogicalIndex);
    mSelected.setSortRole(kVisualIndex);

    lvUnselected->setModel(&mUnselected);
    lvSelected->setModel(&mSelected);
}

QList<uint> PortStatsFilterDialog::getItemList(bool* ok, 
    QAbstractItemModel *model, Qt::Orientation orientation,
    QList<uint> initial)
{
    QList<uint>    ret;

    uint count = (orientation == Qt::Vertical) ? 
        model->rowCount() : model->columnCount();

    *ok = false;

    mUnselected.clear();
    mSelected.clear();

    for (uint i = 0; i < count; i++)
    {
        QStandardItem    *item;
        
        item = new QStandardItem(model->headerData(i, orientation).toString());
        item->setData(i, kLogicalIndex);
        item->setData(initial.indexOf(i), kVisualIndex);
        item->setFlags(Qt::ItemIsSelectable
                | Qt::ItemIsDragEnabled
                //| Qt::ItemIsDropEnabled
                | Qt::ItemIsEnabled);

        if (initial.contains(i))
            mSelected.appendRow(item);
        else
            mUnselected.appendRow(item);
    }
    mSelected.sort(0);

    // No need to sort right now 'coz we have inserted items in order

    if (exec() == QDialog::Accepted)
    {
        uint count = mSelected.rowCount();
        for (uint i = 0; i < count; i++)
        {
            QModelIndex index = mSelected.index(i, 0, QModelIndex());
            QStandardItem *item = mSelected.itemFromIndex(index);
            ret.append(item->data(kLogicalIndex).toInt());
        }
        *ok = true;
    }

    return ret;
}

void PortStatsFilterDialog::on_tbSelectIn_clicked()
{
    QList<int> rows;

    foreach(QModelIndex idx, lvUnselected->selectionModel()->selectedIndexes())
        rows.append(idx.row());
    std::sort(rows.begin(), rows.end(), qGreater<int>());

    QModelIndex idx = lvSelected->selectionModel()->currentIndex();
    int insertAt = idx.isValid() ? idx.row() : mSelected.rowCount();

    foreach(int row, rows)
    {
        QList<QStandardItem*> items = mUnselected.takeRow(row);
        mSelected.insertRow(insertAt, items);
    }
}

void PortStatsFilterDialog::on_tbSelectOut_clicked()
{
    QList<int> rows;

    foreach(QModelIndex idx, lvSelected->selectionModel()->selectedIndexes())
        rows.append(idx.row());
    std::sort(rows.begin(), rows.end(), qGreater<int>());

    foreach(int row, rows)
    {
        QList<QStandardItem*> items = mSelected.takeRow(row);
        mUnselected.appendRow(items);
    }

    mUnselected.sort(0);
}

void PortStatsFilterDialog::on_lvUnselected_doubleClicked(const QModelIndex &index)
{
    QList<QStandardItem*> items = mUnselected.takeRow(index.row());
    QModelIndex idx = lvSelected->selectionModel()->currentIndex();
    int insertAt = idx.isValid() ? idx.row() : mSelected.rowCount();

    mSelected.insertRow(insertAt, items);
}

void PortStatsFilterDialog::on_lvSelected_doubleClicked(const QModelIndex &index)
{
    QList<QStandardItem*> items = mSelected.takeRow(index.row());
    mUnselected.appendRow(items);
    mUnselected.sort(0);
}

