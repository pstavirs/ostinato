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

#include "applymsg.h"
#include "deviceswidget.h"
#include "fileformat.pb.h"
#include "portconfigdialog.h"
#include "portgrouplist.h"
#include "portwidget.h"
#include "settings.h"
#include "streamswidget.h"

#include <QInputDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSortFilterProxyModel>

extern QMainWindow *mainWindow;

PortsWindow::PortsWindow(PortGroupList *pgl, QWidget *parent)
    : QWidget(parent), proxyPortModel(NULL)
{
    proxyPortModel = new QSortFilterProxyModel(this);

    plm = pgl;

    setupUi(this);
    applyMsg_ = new ApplyMessage();
    portWidget->setPortGroupList(plm);
    streamsWidget->setPortGroupList(plm);
    devicesWidget->setPortGroupList(plm);

    tvPortList->header()->hide();

    // Populate PortList Context Menu Actions
    tvPortList->addAction(actionNew_Port_Group);
    tvPortList->addAction(actionDelete_Port_Group);
    tvPortList->addAction(actionConnect_Port_Group);
    tvPortList->addAction(actionDisconnect_Port_Group);

    tvPortList->addAction(actionExclusive_Control);
    tvPortList->addAction(actionPort_Configuration);

    // PortList, StreamList, DeviceWidget actions combined
    // make this window's actions
    addActions(tvPortList->actions());
    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    addAction(sep);
    addActions(streamsWidget->actions());
    sep = new QAction(this);
    sep->setSeparator(true);
    addAction(sep);
    addActions(devicesWidget->actions());

    // XXX: It would be ideal if we only needed to do the below to 
    // get the proxy model to do its magic. However, the QModelIndex
    // used by the source model and the proxy model are different
    // i.e. the row, column, internalId/internalPtr used by both
    // will be different. Since our domain objects - PortGroupList,
    // PortGroup, Port etc. use these attributes, we need to map the
    // proxy's index to the source's index before invoking any domain
    // object methods
    // TODO: research if we can skip the mapping when the domain 
    // objects' design is reviewed
    if (proxyPortModel) {
        proxyPortModel->setSourceModel(plm->getPortModel());
        tvPortList->setModel(proxyPortModel);
    }
    else
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

    connect(this,
            SIGNAL(currentPortChanged(const QModelIndex&, const QModelIndex&)),
            portWidget, SLOT(setCurrentPortIndex(const QModelIndex&)));
    connect(this,
            SIGNAL(currentPortChanged(const QModelIndex&, const QModelIndex&)),
            streamsWidget, SLOT(setCurrentPortIndex(const QModelIndex&)));
    connect(this,
            SIGNAL(currentPortChanged(const QModelIndex&, const QModelIndex&)),
            devicesWidget, SLOT(setCurrentPortIndex(const QModelIndex&)));

    // Initially we don't have any ports/streams/devices
    //  - so send signal triggers
    when_portView_currentChanged(QModelIndex(), QModelIndex());
}

PortsWindow::~PortsWindow()
{
    delete proxyPortModel;
    delete applyMsg_;
}

int PortsWindow::portGroupCount()
{
    return plm->numPortGroups();
}

int PortsWindow::reservedPortCount()
{
    int count = 0;
    int n = portGroupCount();

    for (int i = 0; i < n; i++)
        count += plm->portGroupByIndex(i).numReservedPorts();

    return count;
}

//! Always return true
bool PortsWindow::openSession(
        const OstProto::SessionContent *session,
        QString & /*error*/)
{
    QProgressDialog progress("Opening Session", NULL,
                             0, session->port_groups_size(), mainWindow);
    progress.show();
    progress.setEnabled(true); // since parent (mainWindow) is disabled

    plm->removeAllPortGroups();

    for (int i = 0; i < session->port_groups_size(); i++) {
        const OstProto::PortGroupContent &pgc = session->port_groups(i);
        PortGroup *pg = new PortGroup(QString::fromStdString(
                                                    pgc.server_name()),
                                      quint16(pgc.server_port()));
        pg->setConfigAtConnect(&pgc);
        plm->addPortGroup(*pg);
        progress.setValue(i+1);
    }

    return true;
}

/*!
 * Prepare content to be saved for a session
 *
 * If port reservation is in use, saves only 'my' reserved ports
 *
 * Returns false, if user cancels op; true, otherwise
 */
bool PortsWindow::saveSession(
        OstProto::SessionContent *session, // OUT param
        QString & /*error*/,
        QProgressDialog *progress)
{
    int n = portGroupCount();
    QString myself;

    if (progress) {
        progress->setLabelText("Preparing Ports and PortGroups ...");
        progress->setRange(0, n);
    }

    if (reservedPortCount())
        myself = appSettings->value(kUserKey, kUserDefaultValue).toString();

    for (int i = 0; i < n; i++)
    {
        PortGroup &pg = plm->portGroupByIndex(i);
        OstProto::PortGroupContent *pgc = session->add_port_groups();

        pgc->set_server_name(pg.serverName().toStdString());
        pgc->set_server_port(pg.serverPort());

        for (int j = 0; j < pg.numPorts(); j++)
        {
            if (myself != pg.mPorts.at(j)->userName())
                continue;

            OstProto::PortContent *pc = pgc->add_ports();
            OstProto::Port *p = pc->mutable_port_config();

            // XXX: We save the entire OstProto::Port even though some
            // fields may be ephemeral; while opening we use only relevant
            // fields
            pg.mPorts.at(j)->protoDataCopyInto(p);

            for (int k = 0; k < pg.mPorts.at(j)->numStreams(); k++)
            {
                OstProto::Stream *s = pc->add_streams();
                pg.mPorts.at(j)->streamByIndex(k)->protoDataCopyInto(*s);
            }

            for (int k = 0; k < pg.mPorts.at(j)->numDeviceGroups(); k++)
            {
                OstProto::DeviceGroup *dg = pc->add_device_groups();
                dg->CopyFrom(*(pg.mPorts.at(j)->deviceGroupByIndex(k)));
            }
        }

        if (progress) {
            if (progress->wasCanceled())
                return false;
            progress->setValue(i);
        }
        if (i % 2 == 0)
            qApp->processEvents();
    }

    return true;
}

QList<QAction*> PortsWindow::portActions()
{
    return tvPortList->actions();
}

QList<QAction*> PortsWindow::streamActions()
{
    return streamsWidget->actions();
}

QList<QAction*> PortsWindow::deviceActions()
{
    return devicesWidget->actions();
}

void PortsWindow::clearCurrentSelection()
{
    tvPortList->selectionModel()->clearCurrentIndex();
    tvPortList->clearSelection();
}

void PortsWindow::showMyReservedPortsOnly(bool enabled)
{
    if (!proxyPortModel)
        return;

    if (enabled) {
        QString rx = "Port Group|\\["
                    + QRegExp::escape(appSettings->value(kUserKey, 
                                            kUserDefaultValue).toString())
                    + "\\]";
        qDebug("%s: regexp: <%s>", __FUNCTION__, qPrintable(rx));
        proxyPortModel->setFilterRegExp(QRegExp(rx));
    }
    else
        proxyPortModel->setFilterRegExp(QRegExp(""));
}

void PortsWindow::when_portView_currentChanged(const QModelIndex& currentIndex,
    const QModelIndex& previousIndex)
{
    QModelIndex current = currentIndex;
    QModelIndex previous = previousIndex;

    if (proxyPortModel) {
        current = proxyPortModel->mapToSource(current);
        previous = proxyPortModel->mapToSource(previous);
    }

    updatePortViewActions(currentIndex);

    qDebug("In %s", __FUNCTION__);

    if (previous.isValid() && plm->isPort(previous))
    {
        disconnect(&(plm->port(previous)),
                   SIGNAL(localConfigChanged(int, int, bool)),
                   this,
                   SLOT(updateApplyHint(int, int, bool)));
    }

    if (!current.isValid())
    {    
        qDebug("setting stacked widget to welcome page");
        swDetail->setCurrentIndex(0); // welcome page
    }
    else
    {
        if (plm->isPortGroup(current))
        {
            swDetail->setCurrentIndex(1);    // portGroup detail page    
        }
        else if (plm->isPort(current))
        {
            swDetail->setCurrentIndex(2);    // port detail page
            connect(&(plm->port(current)),
                    SIGNAL(localConfigChanged(int, int, bool)),
                    SLOT(updateApplyHint(int, int, bool)));
            if (plm->port(current).isDirty())
                updateApplyHint(plm->port(current).portGroupId(),
                                plm->port(current).id(), true);
            else if (plm->port(current).numStreams())
                applyHint->setText("Click <img src=':/icons/control_play'/> "
                                   "to transmit packets");
            else
                applyHint->setText("");
        }
    }

    emit currentPortChanged(current, previous);
}

void PortsWindow::when_portModel_dataChanged(const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    qDebug("In %s %d:(%d, %d) - %d:(%d, %d)", __FUNCTION__,
            topLeft.parent().isValid(), topLeft.row(), topLeft.column(),
            bottomRight.parent().isValid(), bottomRight.row(), bottomRight.column());

    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    if (topLeft.parent() != bottomRight.parent())
        return;

    // If a port has changed, expand the port group
    if (topLeft.parent().isValid())
        tvPortList->expand(proxyPortModel ? 
                proxyPortModel->mapFromSource(topLeft.parent()) : 
                topLeft.parent());

#if 0 // not sure why the >= <= operators are not overloaded in QModelIndex
    if ((tvPortList->currentIndex() >= topLeft) &&
        (tvPortList->currentIndex() <= bottomRight))
#endif
    if (((topLeft < tvPortList->currentIndex()) || 
            (topLeft == tvPortList->currentIndex())) &&
        (((tvPortList->currentIndex() < bottomRight)) || 
            (tvPortList->currentIndex() == bottomRight)))
    {
        // Update UI to reflect potential change in exclusive mode,
        // transmit mode et al
        when_portView_currentChanged(tvPortList->currentIndex(),
                tvPortList->currentIndex());
    }
}

void PortsWindow::when_portModel_reset()
{
    when_portView_currentChanged(QModelIndex(), tvPortList->currentIndex());
}

void PortsWindow::updateApplyHint(int /*portGroupId*/, int /*portId*/,
        bool configChanged)
{
    if (configChanged)
        applyHint->setText("Configuration has changed - "
                           "<font color='red'><b>click Apply</b></font> "
                           "to activate the changes");
    else if (plm->getStreamModel()->rowCount() > 0)
        applyHint->setText("Configuration activated - "
                           "click <img src=':/icons/control_play'/> "
                           "to transmit packets");
    else
        applyHint->setText("Configuration activated");
}

void PortsWindow::updatePortViewActions(const QModelIndex& currentIndex)
{
    QModelIndex current = currentIndex;

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (!current.isValid())
    {
        qDebug("current is now invalid");
        actionDelete_Port_Group->setDisabled(true);
        actionConnect_Port_Group->setDisabled(true);
        actionDisconnect_Port_Group->setDisabled(true);

        actionExclusive_Control->setDisabled(true);
        actionPort_Configuration->setDisabled(true);
    
        goto _EXIT;
    }

    qDebug("currentChanged %p", (void*)current.internalId());

    if (plm->isPortGroup(current))
    {
        actionDelete_Port_Group->setEnabled(true);

        actionExclusive_Control->setDisabled(true);
        actionPort_Configuration->setDisabled(true);

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
        actionPort_Configuration->setEnabled(true);
    }

_EXIT:
    return;
}

void PortsWindow::on_pbApply_clicked()
{
    QModelIndex    curPort;
    QModelIndex curPortGroup;

    curPort = tvPortList->selectionModel()->currentIndex();
    if (proxyPortModel)
        curPort = proxyPortModel->mapToSource(curPort);
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

    disconnect(applyMsg_);
    connect(&(plm->portGroup(curPortGroup)), SIGNAL(applyFinished()),
            applyMsg_, SLOT(hide()));
    applyMsg_->show();

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
        "Add Port Group", "Port Group Address (HostName[:Port])",
        QLineEdit::Normal, lastNewPortGroup, &ok);
    
    if (ok)
    {
        QStringList addr = text.split(":");
        quint16 port = DEFAULT_SERVER_PORT;

        if (addr.size() > 2) { // IPv6 Address
            // IPv6 addresses with port number SHOULD be specified as
            // [2001:db8::1]:80 (RFC5952 Sec6) to avoid ambiguity due to ':'
            addr = text.split("]:");
            if (addr.size() > 1)
                port = addr[1].toUShort();
        }
        else if (addr.size() == 2) // Hostname/IPv4 + Port specified
            port = addr[1].toUShort();

        // Play nice and remove square brackets irrespective of addr type
        addr[0].remove(QChar('['));
        addr[0].remove(QChar(']'));

        PortGroup *pg = new PortGroup(addr[0], port);
        plm->addPortGroup(*pg);
        lastNewPortGroup = text;
    }
}

void PortsWindow::on_actionDelete_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (current.isValid())
        plm->removePortGroup(plm->portGroup(current));
}

void PortsWindow::on_actionConnect_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (current.isValid())
        plm->portGroup(current).connectToHost();
}

void PortsWindow::on_actionDisconnect_Port_Group_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (current.isValid())
        plm->portGroup(current).disconnectFromHost();
}

void PortsWindow::on_actionExclusive_Control_triggered(bool checked)
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (plm->isPort(current))
    {
        OstProto::Port config;
        
        config.set_is_exclusive_control(checked);
        plm->portGroup(current.parent()).modifyPort(current.row(), config);
    }
}

void PortsWindow::on_actionPort_Configuration_triggered()
{
    QModelIndex    current = tvPortList->selectionModel()->currentIndex();

    if (proxyPortModel)
        current = proxyPortModel->mapToSource(current);

    if (!plm->isPort(current))
        return;

    Port &port = plm->port(current);
    OstProto::Port config;
    // XXX: we don't call Port::protoDataCopyInto() to get config b'coz
    // we want only the modifiable fields populated to send to Drone
    // TODO: extend Port::protoDataCopyInto() to accept an optional param
    // which says copy only modifiable fields
    //plm->port(current).protoDataCopyInto(&config);
    config.set_transmit_mode(port.transmitMode());
    config.set_is_tracking_stream_stats(port.trackStreamStats());
    config.set_is_exclusive_control(port.hasExclusiveControl());
    config.set_user_name(port.userName().toStdString());

    PortConfigDialog dialog(config, port.getStats().state(), this);

    if (dialog.exec() == QDialog::Accepted)
        plm->portGroup(current.parent()).modifyPort(current.row(), config);
}
