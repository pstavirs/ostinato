/*
Copyright (C) 2023 Srivats P.

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

#ifndef _ROW_BORDER_DELEGATE
#define _ROW_BORDER_DELEGATE

#include <QStyledItemDelegate>

#include <QSet>

class RowBorderDelegate : public QStyledItemDelegate
{
public:
    RowBorderDelegate(QSet<int> rows, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), rows_(rows)
    {
    }

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
	QStyledItemDelegate::paint(painter, option, index);
	if (rows_.contains(index.row())) {
	    const QRect rect(option.rect);
	    painter->drawLine(rect.topLeft(), rect.topRight());
	}
    }

    QSet<int> rows_;
};

#endif

