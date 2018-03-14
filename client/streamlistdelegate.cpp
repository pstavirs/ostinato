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

#include <QComboBox>
#include <QCheckBox>
#include <QApplication>
#include <QMouseEvent>

#include "streammodel.h"
#include "streamlistdelegate.h"

StreamListDelegate::StreamListDelegate(QObject *parent)
: QItemDelegate(parent)
{
}


QWidget *StreamListDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    QWidget    *editor = NULL;

    switch ((StreamModel::StreamFields) index.column())
    {
        case StreamModel::StreamStatus:
        {
            editor = new QCheckBox(parent);
            goto _handled;
        }
        case StreamModel::StreamNextWhat:
        {
            editor = new QComboBox(parent);
            static_cast<QComboBox*>(editor)->insertItems(0, 
                StreamModel::nextWhatOptionList());
            goto _handled;
        }

        case StreamModel::StreamIcon:
        case StreamModel::StreamName:
        default:
            break;
    }

    editor =  QItemDelegate::createEditor(parent, option, index);

_handled:
    return editor;

}


void StreamListDelegate::setEditorData(QWidget *editor,
    const QModelIndex &index) const
{
    switch ((StreamModel::StreamFields) index.column())
    {
        case StreamModel::StreamStatus:
        {
            QCheckBox *cb = static_cast<QCheckBox*>(editor);
            cb->setChecked(
                index.model()->data(index, Qt::EditRole).toBool());
            goto _handled;
        }
        case StreamModel::StreamNextWhat:
        {
            QComboBox *cb = static_cast<QComboBox*>(editor);
            cb->setCurrentIndex(
                index.model()->data(index, Qt::EditRole).toInt());
            goto _handled;
        }

        case StreamModel::StreamIcon:
        case StreamModel::StreamName:
        default:
            break;
    }

    QItemDelegate::setEditorData(editor, index);

_handled:
    return;
}


void StreamListDelegate::setModelData(QWidget *editor,
        QAbstractItemModel *model, const QModelIndex &index) const
{
    switch ((StreamModel::StreamFields) index.column())
    {
        case StreamModel::StreamStatus:
        {
            QCheckBox *cb = static_cast<QCheckBox*>(editor);
            model->setData(index, cb->isChecked(), Qt::EditRole);
            goto _handled;
        }

        case StreamModel::StreamNextWhat:
        {
            QComboBox *cb = static_cast<QComboBox*>(editor);
            model->setData(index, cb->currentIndex(), Qt::EditRole);
            goto _handled;
        }

        case StreamModel::StreamIcon:
        case StreamModel::StreamName:
        default:
            break;
    }

    QItemDelegate::setModelData(editor, model, index);

_handled:
    return;
}


void StreamListDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    switch ((StreamModel::StreamFields) index.column())
    {
        case StreamModel::StreamStatus:
        {
            /*
             * extra 'coz QItemDelegate does it - otherwise the editor
             * placement is incorrect
             */
            int extra = 2 * (qApp->style()->pixelMetric(
                QStyle::PM_FocusFrameHMargin, 0) + 1);

            editor->setGeometry(option.rect.translated(extra, 0));
            goto _handled;
        }
        case StreamModel::StreamIcon:
        case StreamModel::StreamName:
        case StreamModel::StreamNextWhat:
        default:
            break;
    }

    QItemDelegate::updateEditorGeometry(editor, option, index);

_handled:
    return;
} 


bool StreamListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
        const QStyleOptionViewItem &option, const QModelIndex &index) 
{
    /* 
     * Special Handling so that user can use the "Stream status" checkbox 
     * without double clicking first. Copied from QItemDelegate::editorEvent()
     * and modified suitably
     */
    if ((StreamModel::StreamFields)index.column() ==
            StreamModel::StreamStatus)
    {
        // make sure that we have the right event type
        if ((event->type() == QEvent::MouseButtonRelease)
            || (event->type() == QEvent::MouseButtonDblClick))
        {
            QRect checkRect = doCheck(option, option.rect, Qt::Checked);
            QRect emptyRect;
            doLayout(option, &checkRect, &emptyRect, &emptyRect, false);
            if (!checkRect.contains(static_cast<QMouseEvent*>(event)->pos()))
                return false;

            Qt::CheckState state = (static_cast<Qt::CheckState>(index.data(
                Qt::CheckStateRole).toInt()) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            return model->setData(index, state, Qt::CheckStateRole);
        }
    }

    return QItemDelegate::editorEvent(event, model, option, index);
}

