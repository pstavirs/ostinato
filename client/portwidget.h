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

#ifndef _PORT_WIDGET_H
#define _PORT_WIDGET_H

#include "ui_portwidget.h"
#include <QModelIndex>
#include <QWidget>

class PortGroupList;

class PortWidget : public QWidget, private Ui::PortWidget
{
    Q_OBJECT

public:
    PortWidget(QWidget *parent = 0);
    ~PortWidget();

    void setPortGroupList(PortGroupList *portGroups);

public slots:
    void setCurrentPortIndex(const QModelIndex &currentIndex,
                             const QModelIndex &previousIndex);

private slots:


    void on_startTx_clicked();
    void on_stopTx_clicked();
    void on_averageLoadPercent_editingFinished();
    void on_averagePacketsPerSec_editingFinished();
    void on_averageBitsPerSec_editingFinished();

    void updatePortActions();
    void updatePortRates();
    
private:
    PortGroupList *plm{nullptr}; // FIXME: rename to portGroups_?
    QModelIndex currentPortIndex_;
};

#endif

