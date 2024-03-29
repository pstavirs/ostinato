/*
Copyright (C) 2016 Srivats P.

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

#ifndef _STREAM_STATS_WINDOW_H
#define _STREAM_STATS_WINDOW_H

#include "ui_streamstatswindow.h"

class QAbstractItemModel;
class QSortFilterProxyModel;

class StreamStatsWindow: public QWidget, private Ui::StreamStatsWindow
{
    Q_OBJECT
public:
    StreamStatsWindow(QAbstractItemModel *model, QWidget *parent = 0);
    ~StreamStatsWindow();

private slots:
    void on_actionShowDetails_triggered(bool checked);

private:
    QString kDefaultFilter_{"^(?!Port).*"};
    QSortFilterProxyModel *filterModel_;
};

#endif

