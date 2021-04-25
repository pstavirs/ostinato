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

#ifndef _LOGS_WINDOW_H
#define _LOGS_WINDOW_H

#include "ui_logswindow.h"

class LogsModel;
class QDockWidget;
class QShowEvent;
class QMovie;
class QPropertyAnimation;

class LogsWindow: public QWidget, private Ui::LogsWindow
{
    Q_OBJECT
public:
    LogsWindow(LogsModel *model, QWidget *parent = 0);
    ~LogsWindow();

public slots:
    void clearCurrentSelection();

private slots:
    void when_visibilityChanged(bool visible);
    void when_rowsInserted(const QModelIndex &parent, int first, int last);
    void on_autoScroll_toggled(bool checked);

private:
    enum State {kInfo, kWarning, kError};

    QLabel* tabIcon();
    State state();
    void setState(State state);
    void alert(State state);
    void notify();

    State state_{kInfo};
    QDockWidget *parentDock_;
    QMovie *warnAnime_{nullptr};
    QMovie *errorAnime_{nullptr};
    QLabel *alert_{nullptr};
    QPropertyAnimation *alertAnime_{nullptr};
    QString windowTitle_;
    bool isVisible_{false}; // see XXX below
    QObject *logsModelTest_;

    // XXX: We cannot use isVisible() instead of isVisible_ since
    // LogsWindow::isVisible() returns true even when the parent LogsDock
    // is tabified but not the selected tab
};

#endif

