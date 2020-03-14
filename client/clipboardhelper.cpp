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

#include "clipboardhelper.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QIcon>

ClipboardHelper::ClipboardHelper(QObject *parent)
    : QObject(parent)
{
    actionCut_ = new QAction(tr("&Cut"), this);
    actionCut_->setObjectName(QStringLiteral("actionCut"));
    actionCut_->setIcon(QIcon(QString::fromUtf8(":/icons/cut.png")));

    actionCopy_ = new QAction(tr("Cop&y"), this);
    actionCopy_->setObjectName(QStringLiteral("actionCopy"));
    actionCopy_->setIcon(QIcon(QString::fromUtf8(":/icons/copy.png")));

    actionPaste_ = new QAction(tr("&Paste"), this);
    actionPaste_->setObjectName(QStringLiteral("actionPaste"));
    actionPaste_->setIcon(QIcon(QString::fromUtf8(":/icons/paste.png")));

    connect(actionCut_, SIGNAL(triggered()), SLOT(actionTriggered()));
    connect(actionCopy_, SIGNAL(triggered()), SLOT(actionTriggered()));
    connect(actionPaste_, SIGNAL(triggered()), SLOT(actionTriggered()));

#if 0 // FIXME: doesn't work correctly yet
    connect(qGuiApp, SIGNAL(focusChanged(QWidget*, QWidget*)),
            SLOT(updateClipboardActions()));
#endif
}

QList<QAction*> ClipboardHelper::actions()
{
    QList<QAction*> actionList({actionCut_, actionCopy_, actionPaste_});
    return actionList;
}

void ClipboardHelper::actionTriggered()
{
    QWidget *focusWidget = QApplication::focusWidget();

    if  (!focusWidget)
        return;

    // single slot to handle cut/copy/paste - find which action was triggered
    QString action = sender()->objectName()
                        .remove("action").append("()").toLower();
    if (focusWidget->metaObject()->indexOfSlot(qPrintable(action)) < 0) {
        qDebug("%s slot not found for object %s:%s ",
                qPrintable(action),
                qPrintable(focusWidget->objectName()),
                focusWidget->metaObject()->className());
        return;
    }

    action.remove("()");
    QMetaObject::invokeMethod(focusWidget, qPrintable(action),
            Qt::DirectConnection);
}

void ClipboardHelper::updateActionStatus()
{
    QWidget *focusWidget = QApplication::focusWidget();
    if  (!focusWidget)
        return;

    qDebug("In %s", __FUNCTION__);

    const QMetaObject *meta = focusWidget->metaObject();
    // FIXME: we should check if the mimeData's mimeType can be pasted in
    // the focusWidget
    actionPaste_->setEnabled(qGuiApp->clipboard()->mimeData()
            && (meta->indexOfSlot("paste()") >= 0));

    bool hasSelection = false;
    if (meta->indexOfProperty("hasSelectedText") >= 0)
        hasSelection |= focusWidget->property("hasSelectedText").toBool();

    bool ret = false;
    if (meta->indexOfMethod("hasSelection()") >= 0) {
        if (QMetaObject::invokeMethod(focusWidget, "hasSelection",
                    Qt::DirectConnection, Q_RETURN_ARG(bool, ret)))
            hasSelection |= ret;
    }

    actionCut_->setEnabled(hasSelection && (meta->indexOfSlot("cut") >= 0));
    actionCopy_->setEnabled(hasSelection && (meta->indexOfSlot("copy") >= 0));
}

