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

#include "streamswidget.h"

#include "clipboardhelper.h"
#include "findreplace.h"
#include "portgrouplist.h"
#include "streamconfigdialog.h"
#include "streamfileformat.h"
#include "streamlistdelegate.h"

#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>

extern ClipboardHelper *clipboardHelper;

StreamsWidget::StreamsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    delegate = new StreamListDelegate;
    tvStreamList->setItemDelegate(delegate);

    tvStreamList->verticalHeader()->setDefaultSectionSize(
            tvStreamList->verticalHeader()->minimumSectionSize());

    // Populate StreamList Context Menu Actions
    tvStreamList->addAction(actionNew_Stream);
    tvStreamList->addAction(actionEdit_Stream);
    tvStreamList->addAction(actionDuplicate_Stream);
    tvStreamList->addAction(actionDelete_Stream);

    QAction *sep2 = new QAction(this);
    sep2->setSeparator(true);
    tvStreamList->addAction(sep2);

    tvStreamList->addAction(actionFind_Replace);

    QAction *sep3 = new QAction(this);
    sep3->setSeparator(true);
    tvStreamList->addAction(sep3);

    tvStreamList->addAction(actionOpen_Streams);
    tvStreamList->addAction(actionSave_Streams);

    // StreamWidget's actions is an aggegate of all sub-widget's actions
    addActions(tvStreamList->actions());

    // Add the clipboard actions to the context menu of streamList
    // but not to StreamsWidget's actions since they are already available
    // in the global Edit Menu
    QAction *sep4 = new QAction("Clipboard", this);
    sep4->setSeparator(true);
    tvStreamList->insertAction(sep2, sep4);
    tvStreamList->insertActions(sep2, clipboardHelper->actions());
}

void StreamsWidget::setPortGroupList(PortGroupList *portGroups)
{
    plm = portGroups;

    tvStreamList->setModel(plm->getStreamModel());

    connect(plm->getStreamModel(), SIGNAL(rowsInserted(QModelIndex, int, int)), 
        SLOT(updateStreamViewActions()));
    connect(plm->getStreamModel(), SIGNAL(rowsRemoved(QModelIndex, int, int)), 
        SLOT(updateStreamViewActions()));

    connect(tvStreamList->selectionModel(), 
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
        SLOT(updateStreamViewActions()));
    connect(tvStreamList->selectionModel(), 
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        SLOT(updateStreamViewActions()));

    tvStreamList->resizeColumnToContents(StreamModel::StreamIcon);
    tvStreamList->resizeColumnToContents(StreamModel::StreamStatus);

    updateStreamViewActions();

    connect(plm->getStreamModel(), 
        SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), 
        this, SLOT(streamModelDataChanged()));
    connect(plm->getStreamModel(), 
        SIGNAL(modelReset()), 
        this, SLOT(streamModelDataChanged()));
}

void StreamsWidget::streamModelDataChanged()
{
    if (plm->isPort(currentPortIndex_))
        plm->port(currentPortIndex_).recalculateAverageRates();
}

StreamsWidget::~StreamsWidget()
{
    delete delegate;
}

void StreamsWidget::on_tvStreamList_activated(const QModelIndex & index)
{
    if (!index.isValid())
    {
        qDebug("%s: invalid index", __FUNCTION__);
        return;
    }

    qDebug("stream list activated\n");

    Port &curPort = plm->port(currentPortIndex_);

    QList<Stream*> streams;
    streams.append(curPort.mutableStreamByIndex(index.row(), false));

    StreamConfigDialog scd(streams, curPort, this);
    if (scd.exec() == QDialog::Accepted) {
        curPort.recalculateAverageRates();
        curPort.setLocalConfigChanged(true);
    }
}

void StreamsWidget::setCurrentPortIndex(const QModelIndex &portIndex)
{
    if (!plm)
        return;

    // XXX: We assume portIndex corresponds to sourceModel, not proxyModel
    if (!plm->isPort(portIndex))
        return;

    qDebug("In %s", __FUNCTION__);

    currentPortIndex_ = portIndex;
    plm->getStreamModel()->setCurrentPortIndex(portIndex);

    updateStreamViewActions();
}

void StreamsWidget::updateStreamViewActions()
{
    // For some reason hasSelection() returns true even if selection size is 0
    // so additional check for size introduced
    if (tvStreamList->selectionModel()->hasSelection() &&
        (tvStreamList->selectionModel()->selection().size() > 0))
    {
        qDebug("Has selection %d",
            tvStreamList->selectionModel()->selection().size());

        // If more than one non-contiguous ranges selected,
        // disable "New" and "Edit"
        if (tvStreamList->selectionModel()->selection().size() > 1)
        {
            actionNew_Stream->setDisabled(true);
            actionEdit_Stream->setDisabled(true);
        }
        else
        {
            actionNew_Stream->setEnabled(true);
            actionEdit_Stream->setEnabled(true);
        }

        // Duplicate/Delete are always enabled as long as we have a selection
        actionDuplicate_Stream->setEnabled(true);
        actionDelete_Stream->setEnabled(true);
    }
    else
    {
        qDebug("No selection");
        if (plm->isPort(currentPortIndex_))
            actionNew_Stream->setEnabled(true);
        else
            actionNew_Stream->setDisabled(true);
        actionEdit_Stream->setDisabled(true);
        actionDuplicate_Stream->setDisabled(true);
        actionDelete_Stream->setDisabled(true);
    }

    actionFind_Replace->setEnabled(tvStreamList->model()->rowCount() > 0);

    actionOpen_Streams->setEnabled(plm->isPort(currentPortIndex_));
    actionSave_Streams->setEnabled(tvStreamList->model()->rowCount() > 0);
}

void StreamsWidget::on_actionNew_Stream_triggered()
{
    qDebug("New Stream Action");

    QItemSelectionModel* selectionModel = tvStreamList->selectionModel();
    if (selectionModel->selection().size() > 1) {
        qDebug("%s: Unexpected selection size %d, can't add", __FUNCTION__,
                selectionModel->selection().size());
        return;
    }

    // In case nothing is selected, insert 1 row at the end
    StreamModel *streamModel = plm->getStreamModel();
    int row = streamModel->rowCount(), count = 1;

    // In case we have a single range selected; insert as many rows as
    // in the singe selected range before the top of the selected range
    if (selectionModel->selection().size() == 1)
    {
        row = selectionModel->selection().at(0).top();
        count = selectionModel->selection().at(0).height();
    }

    Port &curPort = plm->port(currentPortIndex_);

    QList<Stream*> streams;
    for (int i = 0; i < count; i++)
        streams.append(new Stream);

    StreamConfigDialog scd(streams, curPort, this);
    scd.setWindowTitle(tr("Add Stream"));
    if (scd.exec() == QDialog::Accepted)
        streamModel->insert(row, streams);
}

void StreamsWidget::on_actionEdit_Stream_triggered()
{
    qDebug("Edit Stream Action");

    QItemSelectionModel* streamModel = tvStreamList->selectionModel();
    if (!streamModel->hasSelection())
        return;

    Port &curPort = plm->port(currentPortIndex_);

    QList<Stream*> streams;
    foreach(QModelIndex index, streamModel->selectedRows())
        streams.append(curPort.mutableStreamByIndex(index.row(), false));

    StreamConfigDialog scd(streams, curPort, this);
    if (scd.exec() == QDialog::Accepted) {
        curPort.recalculateAverageRates();
        curPort.setLocalConfigChanged(true);
    }
}

void StreamsWidget::on_actionDuplicate_Stream_triggered()
{
    QItemSelectionModel* model = tvStreamList->selectionModel();

    qDebug("Duplicate Stream Action");

    if (model->hasSelection())
    {
        bool isOk;
        int count = QInputDialog::getInt(this, "Duplicate Streams",
                "Count", 1, 1, 9999, 1, &isOk);

        if (!isOk)
            return;

        QList<int> list;
        foreach(QModelIndex index, model->selectedRows())
            list.append(index.row());
        plm->port(currentPortIndex_).duplicateStreams(list, count);
    }
    else
        qDebug("No selection");
}

void StreamsWidget::on_actionDelete_Stream_triggered()
{
    qDebug("Delete Stream Action");

    QModelIndex        index;

    if (tvStreamList->selectionModel()->hasSelection())
    {
        qDebug("SelectedIndexes %d",
            tvStreamList->selectionModel()->selectedRows().size());
        while(tvStreamList->selectionModel()->selectedRows().size())
        {
            index = tvStreamList->selectionModel()->selectedRows().at(0);
            plm->getStreamModel()->removeRows(index.row(), 1);    
        }
    }
    else
        qDebug("No selection");
}

void StreamsWidget::on_actionFind_Replace_triggered()
{
    qDebug("Find & Replace Action");

    FindReplaceDialog findReplace(this);
    if (findReplace.exec() == QDialog::Accepted) {
        // TODO
    }
}

void StreamsWidget::on_actionOpen_Streams_triggered()
{
    qDebug("Open Streams Action");

    QStringList fileTypes = StreamFileFormat::supportedFileTypes(
                                            StreamFileFormat::kOpenFile);
    QString fileType;
    static QString dirName;
    QString fileName;
    QString errorStr;
    bool append = true;
    bool ret;

    Q_ASSERT(plm->isPort(currentPortIndex_));

    if (fileTypes.size())
        fileType = fileTypes.at(0);

    fileName = QFileDialog::getOpenFileName(this, tr("Open Streams"),
            dirName, fileTypes.join(";;"), &fileType);
    if (fileName.isEmpty())
        goto _exit;

    if (tvStreamList->model()->rowCount())
    {
        QMessageBox msgBox(QMessageBox::Question, qApp->applicationName(),
                tr("Append to existing streams? Or overwrite?"),
                QMessageBox::NoButton, this);
        QPushButton *appendBtn = msgBox.addButton(tr("Append"), 
                QMessageBox::ActionRole);
        QPushButton *overwriteBtn = msgBox.addButton(tr("Overwrite"), 
                QMessageBox::ActionRole);
        QPushButton *cancelBtn = msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        if (msgBox.clickedButton() == cancelBtn)
            goto _exit;
        else if (msgBox.clickedButton() == appendBtn)
            append = true;
        else if (msgBox.clickedButton() == overwriteBtn)
            append = false;
        else
            Q_ASSERT(false);
    }

    ret = plm->port(currentPortIndex_).openStreams(fileName, append, errorStr);
    if (!ret || !errorStr.isEmpty())
    {
        QMessageBox msgBox(this);
        QStringList str = errorStr.split("\n\n\n\n");

        msgBox.setIcon(ret ? QMessageBox::Warning : QMessageBox::Critical);
        msgBox.setWindowTitle(qApp->applicationName());
        msgBox.setText(str.at(0));
        if (str.size() > 1)
            msgBox.setDetailedText(str.at(1));
        msgBox.setStandardButtons(QMessageBox::Ok);

        msgBox.exec();
    }
    dirName = QFileInfo(fileName).absolutePath();
    updateStreamViewActions();

_exit:
    return;
}

void StreamsWidget::on_actionSave_Streams_triggered()
{
    qDebug("Save Streams Action");

    static QString fileName;
    QStringList fileTypes = StreamFileFormat::supportedFileTypes(
                                            StreamFileFormat::kSaveFile);
    QString fileType;
    QString errorStr;
    QFileDialog::Options options;

    // On Mac OS with Native Dialog, getSaveFileName() ignores fileType 
    // which we need
#if defined(Q_OS_MAC)
    options |= QFileDialog::DontUseNativeDialog;
#endif

    if (fileTypes.size())
        fileType = fileTypes.at(0);

    Q_ASSERT(plm->isPort(currentPortIndex_));

_retry:
    fileName = QFileDialog::getSaveFileName(this, tr("Save Streams"),
            fileName, fileTypes.join(";;"), &fileType, options);
    if (fileName.isEmpty())
        goto _exit;

    if (QFileInfo(fileName).suffix().isEmpty()) {
        QString fileExt = fileType.section(QRegExp("[\\*\\)]"), 1, 1);
        qDebug("Adding extension '%s' to '%s'",
                qPrintable(fileExt), qPrintable(fileName));
        fileName.append(fileExt);
        if (QFileInfo(fileName).exists()) {
            if (QMessageBox::warning(this, tr("Overwrite File?"), 
                QString("The file \"%1\" already exists.\n\n"
                    "Do you wish to overwrite it?")
                    .arg(QFileInfo(fileName).fileName()),
                QMessageBox::Yes|QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes)
                goto _retry;
        }
    }

    fileType = fileType.remove(QRegExp("\\(.*\\)")).trimmed();
    if (!fileType.startsWith("Ostinato") 
            && !fileType.startsWith("Python"))
    {
        if (QMessageBox::warning(this, tr("Ostinato"), 
            QString("You have chosen to save in %1 format. All stream "
                "attributes may not be saved in this format.\n\n"
                "It is recommended to save in native Ostinato format.\n\n"
                "Continue to save in %2 format?").arg(fileType).arg(fileType),
            QMessageBox::Yes|QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes)
            goto _retry;
    }

    // TODO: all or selected?

    if (!plm->port(currentPortIndex_).saveStreams(fileName, fileType, errorStr))
        QMessageBox::critical(this, qApp->applicationName(), errorStr);
    else if (!errorStr.isEmpty())
        QMessageBox::warning(this, qApp->applicationName(), errorStr);

    fileName = QFileInfo(fileName).absolutePath();
_exit:
    return;
}

