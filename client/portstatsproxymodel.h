/*
Copyright (C) 2015 Srivats P.

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

#ifndef _PORT_STATS_PROXY_MODEL_H
#define _PORT_STATS_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class PortStatsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PortStatsProxyModel(int userRow, QObject *parent = 0)
        : QSortFilterProxyModel(parent), userRow_(userRow)
    {
        setFilterRegExp(QRegExp(".*"));
    }

protected:
    bool filterAcceptsColumn(int sourceColumn, 
                             const QModelIndex &sourceParent) const
    {
        QModelIndex index = sourceModel()->index(userRow_, sourceColumn,sourceParent);
        QString user = sourceModel()->data(index).toString();

        return filterRegExp().exactMatch(user) ? true : false;
    }
    bool filterAcceptsRow(int sourceRow, 
                          const QModelIndex &/*sourceParent*/) const
    {
        return sourceRow == userRow_ ? false : true;
    }
private:
    int userRow_;
};

#endif

