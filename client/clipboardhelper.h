/*
Copyright (C) 2020 Srivats P.

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

#ifndef _CLIPBOARD_HELPER_H
#define _CLIPBOARD_HELPER_H

#include <QList>

class QAction;

class ClipboardHelper : public QObject
{
    Q_OBJECT
public:
    ClipboardHelper(QObject *parent=nullptr);

    QList<QAction*> actions();

private slots:
    void actionTriggered();
    void updateActionStatus();

private:
    QAction *actionCut_{nullptr};
    QAction *actionCopy_{nullptr};
    QAction *actionPaste_{nullptr};
};

#endif
