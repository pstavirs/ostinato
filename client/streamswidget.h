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

#ifndef _STREAMS_WIDGET_H
#define _STREAMS_WIDGET_H

#include "ui_streamswidget.h"
#include <QWidget>

class PortGroupList;
class QAbstractItemDelegate;

class StreamsWidget : public QWidget, private Ui::StreamsWidget
{
    Q_OBJECT

public:
    StreamsWidget(QWidget *parent = 0);
    ~StreamsWidget();

    void setPortGroupList(PortGroupList *portGroups);

public slots:
    void setCurrentPortIndex(const QModelIndex &portIndex);

private slots:
    void updateStreamViewActions();

    void on_tvStreamList_activated(const QModelIndex & index);

    void on_actionNew_Stream_triggered();
    void on_actionEdit_Stream_triggered();
    void on_actionDuplicate_Stream_triggered();
    void on_actionDelete_Stream_triggered();

    void on_actionOpen_Streams_triggered();
    void on_actionSave_Streams_triggered();

    void streamModelDataChanged();

private:
    PortGroupList *plm{nullptr}; // FIXME: rename to portGroups_?
    QModelIndex currentPortIndex_;

    QAbstractItemDelegate *delegate;
};

#endif

