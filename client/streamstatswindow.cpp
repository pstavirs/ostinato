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

#include "streamstatswindow.h"

#include "streamstatsfiltermodel.h"

#include <QAbstractItemModel>
#include <QHeaderView>

static int id;
static int count;

StreamStatsWindow::StreamStatsWindow(QAbstractItemModel *model, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    streamStats->addAction(actionShowDetails);

    if (id)
        setWindowTitle(windowTitle() + QString("(%1)").arg(id));
    id++;
    count++;

    filterModel_ = new StreamStatsFilterModel(this);
    filterModel_->setFilterRegExp(QRegExp(kDefaultFilter_));
    filterModel_->setSourceModel(model);
    streamStats->setModel(filterModel_);

    streamStats->verticalHeader()->setHighlightSections(false);
    streamStats->verticalHeader()->setDefaultSectionSize(
            streamStats->verticalHeader()->minimumSectionSize());
}

StreamStatsWindow::~StreamStatsWindow()
{
    delete filterModel_;
    count--;
    if (count == 0)
        id = 0;
}

void StreamStatsWindow::on_actionShowDetails_triggered(bool checked)
{
    if (checked)
        filterModel_->setFilterRegExp(QRegExp(".*"));
    else
        filterModel_->setFilterRegExp(QRegExp(kDefaultFilter_));
}
