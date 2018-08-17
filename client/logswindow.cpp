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

#include "logswindow.h"

#include "logsmodel.h"

#include <QHeaderView>

LogsWindow::LogsWindow(LogsModel *model, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    logs->verticalHeader()->setHighlightSections(false);
    logs->verticalHeader()->setDefaultSectionSize(
            logs->verticalHeader()->minimumSectionSize());
    logs->setShowGrid(false);
    logs->setAlternatingRowColors(true);
    logs->setModel(model);
    logs->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    //FIXME: connect(clear, SIGNAL(clicked()), model, SLOT(clear()));
}

LogsWindow::~LogsWindow()
{
}

