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

#include "portswindow.h"

#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>

#include "streamconfigdialog.h"
#include "streamlistdelegate.h"

PortsWindow::PortsWindow(PortGroupList *pgl, QWidget *parent)
    : QWidget(parent)
{
    delegate = new StreamListDelegate;
    //slm = new StreamListModel();
    //plm = new PortGroupList();
    plm = pgl;

    setupUi(this);

    tvPortList->header()->hide();

    tvStreamList->setItemDelegate(delegate);

    tvStreamList->verticalHeader()->setDefaultSectionSize(
            tvStreamList->verticalHeader()->minimumSectionSize());

    // Populate Context Menu Actions
    tvPortList->addAction(actionNew_Port_Group);
    tvPortList->addAction(actionDelete_Port_Group);
    tvPortList->addAction(actionConnect_Port_Group);
    tvPortList->addAction(actionDisconnect_Port_Group);

    tvPortList->addAction(actionExclusive_Control);

    tvStreamList->addAction(actionNew_Stream);
    tvStreamList->addAction(actionEdit_Stream);
    tvStreamList->addAction(actionDelete_Stream);

    addActions(tvPortList->actions());
    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    addAction(sep);
    addActions(tvStreamList->actions());

    tvStreamList->setModel(plm->getStreamModel());
    tvPortList->setModel(plm->getPortModel());
    
    connect( plm->getPortModel(), 
        SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), 
        this, SLOT(when_portModel_dataChanged(const QModelIndex&, 
            const QModelIndex&)));

    connect(plm->getPortModel(), SIGNAL(modelReset()), 
        SLOT(when_portModel_reset()));

    connect( tvPortList->selectionModel(), 
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
        this, SLOT(when_portView_currentChanged(const QModelIndex&, 
            const QModelIndex&)));

    connect( tvStreamList->selectionModel(), 
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
        this, SLOT(when_streamView_currentChanged(const QModelIndex&, 
            const QModelIndex&)));
    connect( tvStreamList->selectionModel(), 
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(when_streamView_selectionChanged()));

#if 0
    connect( tvPortList->selectionModel(), 
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
        plm->getStreamModel(), SLOT(setCurrentPortIndex(const QModelIndex&)));
#endif

    tvStreamList->resizeColumnToContents(StreamModel::StreamIcon);
    tvStreamList->resizeColumnToContents(StreamModel::StreamStatus);

    // Initially we don't have any ports/streams - so send signal triggers
    when_portView_currentChanged(QModelIndex(), QModelIndex());
    when_streamView_currentChanged(QModelIndex(), QModelIndex());

    //! \todo Hide the Aggregate Box till we add support
    frAggregate->setHidden(true);
}

PortsWindow::~PortsWindow()
{
    delete delegate;
}

void PortsWindow::on_tvStreamList_activated(const QModelIndex & index)
{
    StreamConfigDialog    *scd;

    if (!index.isValid())
    {
        qDebug("%s: invalid index", __FUNCTION__);
        return;
    }
    scd = new StreamConfigDialog(plm->port(tvPortList->currentIndex()),
        index.row(), this);
    qDebug("stream list activated\n");
    scd->exec(); // TODO: chk retval
    delete scd;
}

void PortsWindow::when_portView_currentChanged(const QModelIndex& current,
    const QModelIndex& /*previous*/)
{
    plm->getStreamModel()->setCurrentPortIndex(current);
    updatePortViewActions(current);
    updateStreamViewActions();

    if (!current.isValid())
    {    
        qDebug("setting stacked widget to blank page");
        swDetail->setCurrentIndex(2); // blank page
    }
    else
    {
        if (plm->isPortGroup(current))
        {
            swDetail->setCurrentIndex(1);    // portGroup detail page    
        }
        else if (plm->isPort(current))
        {
            swDetail->setCurrentIndex(0);    // port detail page    
        }
    }
}

void PortsWindow::when_streamView_currentChanged(const QModelIndex& /*current*/,
    const QModelIndex& /*previous*/)
{
    qDebug("stream view current changed");
    updateStreamViewActions();
}

void PortsWindow::when_streamView_selectionChanged()
{
    qDebug("stream view selection changed");
    updateStreamViewActions();
}

void PortsWindow::when_portModel_dataChanged(const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
#if 0 // not sure why the >= <= operators are not overloaded in QModelIndex
    if ((tvPortList->currentIndex() >= topLeft) &&
        (tvPortList->currentIndex() <= bottomRight))
#endif
    if (((topLeft < tvPortList->currentIndex()) || 
            (topLeft == tvPortList->currentIndex())) &&
        (((tvPortList->currentIndex() < bottomRight)) || 
            (tvPortList->currentIndex() == bottomRight)))
    {
        updatePortViewActions(tvPortList->currentIndex());
    }
}

void PortsWindow::when_portModel_reset()
{
    when_portView_currentChanged(QModelIndex(), tvPortList->currentIndex());
}

#if 0
void PortsWindow::updateStreamViewActions(const QModelIndex& current)
{
    if (current.isValid())
        actionDelete_Stream->setEnabled(true);
    else
        actionDelete_Stream->setDisabled(true);
}
#endif

void PortsWindow::updateStreamViewActions()
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

            // Enable "Edit" only if the single range has a single row
            if (tvStreamList->selectionModel()->selection().at(0).height() > 1)
                actionEdit_Stream->setDisabled(true);
            else
                actionEdit_Stream->setEnabled(true);
        }

        // Delete is always enabled as long as we have a selection
        actionDelete_Stream->setEnabled(true);
    }
    else
    {
        qDebug("No selection");
        if (plm->isPort(tvPortList->currentIndex()))
            actionNew_Stream->setEnabled(true);
        else
            actionNew_Stream->setDisabled(true);
        actionEdit_Stream->setDisabled(true);
        actionDelete_Stream->setDisabled(true);
    }
}

void PortsWindow::updatePortViewActions(const QModelIndex& current)
{
    if (!current.isValid())
    {
        qDebug("current is now invalid");
        actionDelete_Port_Group->setDisabled(true);
        actionConnect_Port_Group->setDisabled(true);
        actionDisconnect_Port_Group->setDisabled(true);

        actionExclusive_Control->setDisabled(true);
    
        goto _EXIT;
    }

    qDebug("currentChanged %llx", current.internalId());

    if (plm->isPortGroup(current))
    {
        actionDelete_Port_Group->setEnabled(true);

        actionExclusive_Control->setDisabled(true);

        switch(plm->portGroup(current).state())
        {
            case QAbstractSocket::UnconnectedState:
            case QAbstractSocket::ClosingState:
                qDebug("state = unconnected|closing");
                actionConnect_Port_Group->setEnabled(true);
                actionDisconnect_Port_Group->setDisabled(true);
                break;

            case QAbstractSocket::HostLookupState:
            case QAbstractSocket::ConnectingState:
            case QAbstractSocket::ConnectedState:
                qDebug("state = lookup|connecting|connected");
                actionConnect_Port_Group->setDisabled(true);
                actionDisconnect_Port_Group->setEnabled(true);
                break;


            case QAbstractSocket::BoundState:
            case QAbstractSocket::ListeningState:
            default:
                // FIXME(LOW): indicate error
                qDebug("unexpected state");
                break;
        }
    }
    else if (plm->isPort(current))
    {
        actionDelete_Port_Group->setDisabled(true);
        actionConnect_Port_Group->setDisabled(true);
        actionDisconnect_Port_Group->setDisabled(true);

        actionExclusive_Control->setEnabled(true);
        if (plm->port(current).hasExclusiveControl())
            actionExclusive_Control->setChecked(true);
        else
            actionExclusive_Control->setChecked(false);
    }

_EXIT:
    return;
}

void PortsWindow::on_pbApply_clicked()
{
    QModelIndex    curPort;
    QModelIndex curPortGroup;

    curPort = tvPortList->selectionModel()->currentIndex();
    if (!curPort.isValid())
    {
        qDebug("%s: curPort is invalid", __FUNCTION__);
        goto _exit;
    }

    if (!plm->isPort(curPort))
    {
        qDebug("%s: curPort is not a port", __FUNCTION__);
        goto _exit;
    }

    if (plm->port(curPort).getStats().state().is_transmit_on())
    {
        QMessageBox::information(0, "Configuration Change",
                "Please stop transmit on the port before applying any changes");
        goto _exit;
    }

    curPortGroup = plm->getPortModel()->parent(curPort);
    if (!curPortGroup.isValid())
    {
        qDebug("%s: curPortGroup is invalid", __FUNCTION__);
        goto _exit;
    }
    if (!plm->isPortGroup(curPortGroup))
    {
        qDebug("%s: curPortGroup is not a portGroup", __FUNCTION__);
        goto _exit;
    }

    // FIXME(HI): shd this be a signal?
    //portGroup.when_configApply(port);
    // FIXME(MED): mixing port id and index!!!
    plm->portGroup(curPortGroup).when_configApply(plm->port(curPort).id());

_exit:
    return;

#if 0
    // TODO (LOW): This block is for testing only
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (current.isValid())
        qDebug("current =  %llx", current.internalId());
    else
        qDebug("current is invalid");
#endif
}

void PortsWindow::on_actionNew_Port_Group_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, 
        "Add Port Group", "Port Group Address (IP[:Port])",
        QLineEdit::Normal, lastNewPortGroup, &ok);
    
    if (ok)
    {
        QStringList addr = text.split(":");
        if (addr.size() == 1) // Port unspecified
            addr.append(QString().setNum(DEFAULT_SERVER_PORT));
        PortGroup *pg = new PortGroup(QHostAddress(addr[0]),addr[1].toUShort());    
        plm->addPortGroup(*pg);
        lastNewPortGroup = text;
    }
}

void PortsWindow::on_actionDelete_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (current.isValid())
        plm->removePortGroup(plm->portGroup(current));
}

void PortsWindow::on_actionConnect_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (current.isValid())
        plm->portGroup(current).connectToHost();
}

void PortsWindow::on_actionDisconnect_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (current.isValid())
        plm->portGroup(current).disconnectFromHost();
}

void PortsWindow::on_actionExclusive_Control_triggered(bool checked)
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (plm->isPort(current))
        plm->portGroup(current.parent()).modifyPort(current.row(), checked);
}

#if 0
void PortsWindow::on_actionNew_Stream_triggered()
{
    qDebug("New Stream Action");

    int row = 0;

    if (tvStreamList->currentIndex().isValid())
        row = tvStreamList->currentIndex().row();
    plm->getStreamModel()->insertRows(row, 1);    
}

void PortsWindow::on_actionDelete_Stream_triggered()
{
    qDebug("Delete Stream Action");
    if (tvStreamList->currentIndex().isValid())
        plm->getStreamModel()->removeRows(tvStreamList->currentIndex().row(), 1);    
}
#endif

void PortsWindow::on_actionNew_Stream_triggered()
{
    qDebug("New Stream Action");

    // In case nothing is selected, insert 1 row at the top
    int row = 0, count = 1;

    // In case we have a single range selected; insert as many rows as
    // in the singe selected range before the top of the selected range
    if (tvStreamList->selectionModel()->selection().size() == 1)
    {
        row = tvStreamList->selectionModel()->selection().at(0).top();
        count = tvStreamList->selectionModel()->selection().at(0).height();
    }

    plm->getStreamModel()->insertRows(row, count);    
}

void PortsWindow::on_actionEdit_Stream_triggered()
{
    qDebug("Edit Stream Action");

    // Ensure we have only one range selected which contains only one row
    if ((tvStreamList->selectionModel()->selection().size() == 1) &&
        (tvStreamList->selectionModel()->selection().at(0).height() == 1))
    {
        on_tvStreamList_activated(tvStreamList->selectionModel()->
                selection().at(0).topLeft());
    }
}

void PortsWindow::on_actionDelete_Stream_triggered()
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


