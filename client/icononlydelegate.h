/*
Copyright (C) 2018 Srivats P.

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

#ifndef _ICON_ONLY_DELEGATE
#define _ICON_ONLY_DELEGATE

#include <QStyledItemDelegate>

class IconOnlyDelegate : public QStyledItemDelegate
{
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const
    {
        QStyleOptionViewItem opt = option;
        opt.decorationPosition = QStyleOptionViewItem::Top;
        opt.features &= ~QStyleOptionViewItem::HasDisplay;
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

#endif

