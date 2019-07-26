/*
Copyright (C) 2017 Srivats P.

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

#ifndef _X_TABLE_VIEW_H
#define _X_TABLE_VIEW_H

#include <QTableView>

#include <QClipboard>
#include <QKeyEvent>
#include <QPainter>

class XTableView : public QTableView
{
public:
    XTableView(QWidget *parent) : QTableView(parent) {}
    virtual ~XTableView() {}

protected:
    virtual void paintEvent(QPaintEvent *event)
    {
        if (!model()->hasChildren())  {
            QPainter painter(viewport());
            style()->drawItemText(&painter, viewport()->rect(),
                    layoutDirection() | Qt::AlignCenter, palette(),
                    true, whatsThis(), QPalette::WindowText);
        }
        else
            QTableView::paintEvent(event);
    }

    virtual void keyPressEvent(QKeyEvent *event)
    {
        // Copy selection to clipboard (base class copies only current item)
        if (event->matches(QKeySequence::Copy)
                && selectionBehavior() == SelectRows) {
            QString text;
            int lastRow = -1;
            QModelIndexList selected = selectionModel()->selectedIndexes();
            std::sort(selected.begin(), selected.end());
            foreach(QModelIndex index, selected) {
                if (index.row() != lastRow) {
                    if (!text.isEmpty())
                        text.append("\n");
                }
                else
                    text.append("\t");
                text.append(model()->data(index).toString());
                lastRow = index.row();
            }
            qApp->clipboard()->setText(text);
        }
        else
            QTableView::keyPressEvent(event);
    }
};

#endif

