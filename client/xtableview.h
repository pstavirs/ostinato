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

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>

class XTableView : public QTableView
{
    Q_OBJECT

public:
    XTableView(QWidget *parent) : QTableView(parent) {}
    virtual ~XTableView() {}

    void setModel(QAbstractItemModel *model)
    {
        // This is only a heuristic, but works for us
        if (model && model->supportedDropActions() != Qt::IgnoreAction)
            _modelAllowsRemove = true;
        else
            _modelAllowsRemove = false;

        QTableView::setModel(model);
    }

    bool hasSelection() const
    {
        return !selectionModel()->selectedIndexes().isEmpty();
    }

    bool canCut() const
    {
        return _modelAllowsRemove;
    }

    bool canPaste(const QMimeData *data) const
    {
        return model()->canDropMimeData(data, Qt::CopyAction,
                0, 0, QModelIndex());
    }

public slots:
    void cut()
    {
        copy();
        foreach(QItemSelectionRange range, selectionModel()->selection())
            model()->removeRows(range.top(), range.height());
    }

    void copy()
    {
        // Copy selection to clipboard (base class copies only current item)
        // Selection, by default, is in the order in which items were selected
        //  - sort them before copying
        QModelIndexList selected = selectionModel()->selectedIndexes();

        if (selected.isEmpty())
            return;

        std::sort(selected.begin(), selected.end());
        QMimeData *mimeData = model()->mimeData(selected);
        copyPlainText(selected, mimeData);
        qApp->clipboard()->setMimeData(mimeData);
        qDebug("Copied data in %d format(s) to clipboard",
                mimeData->formats().size());
        for (int i = 0; i < mimeData->formats().size(); i++) {
            qDebug("    %d: %s, %d bytes", i+1,
                    qPrintable(mimeData->formats().at(i)),
                    mimeData->data(mimeData->formats().at(i)).size());
        }
    }

    void paste()
    {
        const QMimeData *mimeData = qApp->clipboard()->mimeData();

        if (!mimeData || mimeData->formats().isEmpty())
            return;

        if (selectionModel()->hasSelection()
                && selectionModel()->selection().size() > 1) {
            qWarning("Cannot paste into multiple(%d) selections",
                    selectionModel()->selection().size());
            return;
        }

        // If no selection, insert at the end
        int row, column;
        if (selectionModel()->hasSelection()
                && selectionModel()->selection().size() == 1) {
            row = selectionModel()->selection().first().top();
            column = selectionModel()->selection().first().left();
        } else {
            row = model()->rowCount();
            column = model()->columnCount();
        }

        if (model()->canDropMimeData(mimeData, Qt::CopyAction,
                    row, column, QModelIndex()))
            model()->dropMimeData(mimeData, Qt::CopyAction,
                    row, column, QModelIndex());
    }

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
        if (event->matches(QKeySequence::Cut)) {
            cut();
        } else if (event->matches(QKeySequence::Copy)) {
            copy();
        } else if (event->matches(QKeySequence::Paste)) {
            paste();
        } else
            QTableView::keyPressEvent(event);
    }

private:
    void copyPlainText(const QModelIndexList &indexes, QMimeData *mimeData)
    {
        if (mimeData->hasText())
            return;

        bool includeHeaders = (selectionBehavior()
                                    != QAbstractItemView::SelectItems);
        QString text;
        if (includeHeaders) {
            int start = 0, end = model()->columnCount(); // assume SelectRows
            if (selectionBehavior() == QAbstractItemView::SelectColumns) {
                start = indexes.first().column();
                end = indexes.last().column()+1;
            }
            text.append("\t"); // column header for row number/title
            for (int i = start; i < end; i++)
                if (indexes.contains(model()->index(indexes.first().row(), i)))
                    text.append(model()->headerData(i, Qt::Horizontal)
                                            .toString()+"\t");;
            text.append("\n");
        }

        int lastRow = -1;
        foreach(QModelIndex index, indexes) {
            if (index.row() != lastRow) { // row changed
                if (lastRow >= 0)
                    text.append("\n");
                if (includeHeaders)
                    text.append(model()->headerData(index.row(), Qt::Vertical)
                                            .toString()+"\t");
            }
            else
                text.append("\t");
            text.append(model()->data(index).toString());
            lastRow = index.row();
        }
        text.append("\n");

        mimeData->setText(text);
    }

    bool _modelAllowsRemove{false};
};

#endif

