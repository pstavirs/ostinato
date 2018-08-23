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
#include "modeltest.h"

#include <QDockWidget>
#include <QHeaderView>

LogsWindow::LogsWindow(LogsModel *model, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    logs->setModel(model);
    autoScroll->setChecked(true);

    logs->verticalHeader()->setHighlightSections(false);
    logs->verticalHeader()->setDefaultSectionSize(
            logs->verticalHeader()->minimumSectionSize());
    logs->setShowGrid(false);
    logs->setAlternatingRowColors(true);
    logs->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    parentDock_ = qobject_cast<QDockWidget*>(parent);
    windowTitle_ = parentDock_->windowTitle();

    connect(level, SIGNAL(currentIndexChanged(int)),
            model, SLOT(setLogLevel(int)));
    connect(clear, SIGNAL(clicked()), model, SLOT(clear()));

    connect(parentDock_, SIGNAL(visibilityChanged(bool)),
            SLOT(when_visibilityChanged(bool)));
    connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            SLOT(when_rowsInserted(const QModelIndex&, int, int)));

#if defined(QT_NO_DEBUG) || QT_VERSION < 0x050700
    logsModelTest_ = nullptr;
#else
    logsModelTest_ = new ModelTest(model);
#endif
}

LogsWindow::~LogsWindow()
{
    delete logsModelTest_;
}

void LogsWindow::when_visibilityChanged(bool visible)
{
    if (visible) {
        parentDock_->setWindowTitle(windowTitle_);
        annotation_.clear();
    }

    isVisible_ = visible;
    qDebug("isVisible = %u", isVisible_);
}

void LogsWindow::when_rowsInserted(const QModelIndex &parent,
                                   int first, int last)
{
    if (isVisible_)
        return;

    if (annotation_.contains("Error"))
        return;

    for (int i = first; i <= last; i++) {
        // FIXME: use a user-role instead, so we don't need to know column and
        // have to compare strings?
        QString level = logs->model()->data(logs->model()->index(i, 1, parent))
                                        .toString();
        if (level == "Error") {
            annotation_ = QString(" - Error(s)");
            break; // Highest level - no need to look further
        }
        else if (level == "Warning") {
            annotation_ = QString(" - Warning(s)");
        }
    }
    parentDock_>setWindowTitle(windowTitle_+annotation_);
}

void LogsWindow::on_autoScroll_toggled(bool checked)
{
    if (checked) {
        connect(logs->model(),
                SIGNAL(rowsInserted(const QModelIndex&, int, int)),
                logs, SLOT(scrollToBottom()));
    }
    else {
        disconnect(logs->model(),
                SIGNAL(rowsInserted(const QModelIndex&, int, int)),
                logs, SLOT(scrollToBottom()));
    }
}
