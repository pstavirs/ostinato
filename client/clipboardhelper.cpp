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

#include "xtableview.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QItemSelection>

#if 0 // change 0 to 1 for debug
#define xDebug(...) qDebug(__VA_ARGS__)
#else
#define xDebug(...)
#endif

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

    // XXX: failsafe in case updation of cut/copy/status causes issues
    // Temporary for 1 release - will be removed after that
    if (qEnvironmentVariableIsSet("X-OSTINATO-CCP-STATUS")) {
        qWarning("FAILSAFE: Cut-Copy-Paste action status will not be updated");
        return;
    }

    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)),
            SLOT(updateCutCopyStatus(QWidget*, QWidget*)));

    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)),
            SLOT(updatePasteStatus()));
    connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()),
            SLOT(updatePasteStatus()));
}

QList<QAction*> ClipboardHelper::actions()
{
    QList<QAction*> actionList({actionCut_, actionCopy_, actionPaste_});
    return actionList;
}

void ClipboardHelper::actionTriggered()
{
    QWidget *focusWidget = qApp->focusWidget();

    if  (!focusWidget)
        return;

    // single slot to handle cut/copy/paste - find which action was triggered
    QString action = sender()->objectName()
                        .remove("action").append("()").toLower();
    if (focusWidget->metaObject()->indexOfSlot(qPrintable(action)) < 0) {
        xDebug("%s slot not found for object %s:%s ",
                qPrintable(action),
                qPrintable(focusWidget->objectName()),
                focusWidget->metaObject()->className());
        return;
    }

    action.remove("()");
    QMetaObject::invokeMethod(focusWidget, qPrintable(action),
            Qt::DirectConnection);
}

void ClipboardHelper::updateCutCopyStatus(QWidget *old, QWidget *now)
{
    xDebug("In %s", __FUNCTION__);

    const XTableView *view = dynamic_cast<XTableView*>(old);
    if (view) {
        disconnect(view->selectionModel(),
                   SIGNAL(selectionChanged(const QItemSelection&,
                                           const QItemSelection&)),
                   this,
                   SLOT(focusWidgetSelectionChanged(const QItemSelection&,
                                                    const QItemSelection&)));
        disconnect(view->model(), SIGNAL(modelReset()),
                   this, SLOT(focusWidgetModelReset()));
    }

    if  (!now) {
        xDebug("No focus widget to copy from");
        actionCut_->setEnabled(false);
        actionCopy_->setEnabled(false);
        return;
    }

    const QMetaObject *meta = now->metaObject();
    if (meta->indexOfSlot("copy()") < 0) {
        xDebug("Focus Widget (%s) doesn't have a copy slot",
                qPrintable(now->objectName()));
        actionCut_->setEnabled(false);
        actionCopy_->setEnabled(false);
        return;
    }

    view = dynamic_cast<XTableView*>(now);
    if (view) {
        connect(view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection&,
                                        const QItemSelection&)),
                SLOT(focusWidgetSelectionChanged(const QItemSelection&,
                                                 const QItemSelection&)));
        connect(view->model(), SIGNAL(modelReset()),
                this, SLOT(focusWidgetModelReset()));
        if (!view->hasSelection()) {
            xDebug("%s doesn't have anything selected to copy",
                    qPrintable(view->objectName()));
            actionCut_->setEnabled(false);
            actionCopy_->setEnabled(false);
            return;
        }
        xDebug("%s model can cut: %d", qPrintable(view->objectName()),
                view->canCut());
        actionCut_->setEnabled(view->canCut());
    }

    xDebug("%s has a selection and copy slot: copy possible",
            qPrintable(now->objectName()));
    actionCopy_->setEnabled(true);
}

void ClipboardHelper::focusWidgetSelectionChanged(
        const QItemSelection &selected, const QItemSelection &/*deselected*/)
{
    xDebug("In %s", __FUNCTION__);

    // Selection changed in the XTableView that has focus
    const XTableView *view = dynamic_cast<XTableView*>(qApp->focusWidget());
    xDebug("canCut:%d empty:%d", view->canCut(), selected.indexes().isEmpty());
    actionCut_->setEnabled(!selected.indexes().isEmpty()
            && view && view->canCut());
    actionCopy_->setEnabled(!selected.indexes().isEmpty());
}

void ClipboardHelper::updatePasteStatus()
{
    xDebug("In %s", __FUNCTION__);

    QWidget *focusWidget = qApp->focusWidget();
    if  (!focusWidget) {
        xDebug("No focus widget to paste into");
        actionPaste_->setEnabled(false);
        return;
    }

    const QMimeData *item = QGuiApplication::clipboard()->mimeData();
    if  (!item || item->formats().isEmpty()) {
        xDebug("Nothing on clipboard to paste");
        actionPaste_->setEnabled(false);
        return;
    }

    const QMetaObject *meta = focusWidget->metaObject();
    if (meta->indexOfSlot("paste()") < 0) {
        xDebug("Focus Widget (%s) doesn't have a paste slot",
                qPrintable(focusWidget->objectName()));
        actionPaste_->setEnabled(false);
        return;
    }

    const XTableView *view = dynamic_cast<XTableView*>(focusWidget);
    if (view && !view->canPaste(item)) {
        xDebug("%s cannot accept this item (%s)",
                qPrintable(view->objectName()),
                qPrintable(item->formats().join("|")));
        actionPaste_->setEnabled(false);
        return;
    }

    xDebug("%s can accept this item (%s): paste possible",
            qPrintable(focusWidget->objectName()),
            qPrintable(item->formats().join("|")));
    actionPaste_->setEnabled(true);
}

void ClipboardHelper::focusWidgetModelReset()
{
    xDebug("In %s", __FUNCTION__);

    QWidget *focusWidget = qApp->focusWidget();
    updateCutCopyStatus(focusWidget, focusWidget); // re-eval cut/copy status
}

#undef xDebug

