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

#include "portwidget.h"

#include "portgrouplist.h"
#include "xqlocale.h"

#include <cfloat>

PortWidget::PortWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

void PortWidget::setPortGroupList(PortGroupList *portGroups)
{
    plm = portGroups;

    connect(plm->getStreamModel(), SIGNAL(rowsInserted(QModelIndex, int, int)), 
        SLOT(updatePortActions()));
    connect(plm->getStreamModel(), SIGNAL(rowsRemoved(QModelIndex, int, int)), 
        SLOT(updatePortActions()));
    connect(plm->getStreamModel(), SIGNAL(modelReset()), 
        SLOT(updatePortActions()));

    updatePortActions();
}

PortWidget::~PortWidget()
{
}

void PortWidget::setCurrentPortIndex(const QModelIndex &portIndex)
{
    if (!plm)
        return;

    // XXX: We assume portIndex corresponds to sourceModel, not proxyModel
    if (!plm->isPort(portIndex))
        return;

    qDebug("In %s", __FUNCTION__);

    // Disconnect previous port
    if (plm->isPort(currentPortIndex_))
        disconnect(&(plm->port(currentPortIndex_)),
                SIGNAL(portRateChanged(int, int)),
                this, SLOT(updatePortRates()));

    currentPortIndex_ = portIndex;

    // Connect current port
    if (plm->isPort(currentPortIndex_))
        connect(&(plm->port(currentPortIndex_)),
                SIGNAL(portRateChanged(int, int)),
                this, SLOT(updatePortRates()));

    double speed = plm->port(currentPortIndex_).speed();
    portSpeed->setText(QString("Max %L1 Mbps").arg(speed));

    rbLoad->setVisible(speed > 0);
    averageLoadPercent->setVisible(speed > 0);
    speedSep->setVisible(speed > 0);
    portSpeed->setVisible(speed > 0);

    updatePortRates();
    updatePortActions();
}

void PortWidget::on_startTx_clicked()
{
    Q_ASSERT(plm->isPort(currentPortIndex_));

    QModelIndex curPortGroup = plm->getPortModel()->parent(currentPortIndex_);
    Q_ASSERT(curPortGroup.isValid());
    Q_ASSERT(plm->isPortGroup(curPortGroup));

    QList<uint> portList({plm->port(currentPortIndex_).id()});
    plm->portGroup(curPortGroup).startTx(&portList);
}

void PortWidget::on_stopTx_clicked()
{
    Q_ASSERT(plm->isPort(currentPortIndex_));

    QModelIndex curPortGroup = plm->getPortModel()->parent(currentPortIndex_);
    Q_ASSERT(curPortGroup.isValid());
    Q_ASSERT(plm->isPortGroup(curPortGroup));

    QList<uint> portList({plm->port(currentPortIndex_).id()});
    plm->portGroup(curPortGroup).stopTx(&portList);
}

void PortWidget::on_averageLoadPercent_editingFinished()
{
    Q_ASSERT(plm->isPort(currentPortIndex_));

    plm->port(currentPortIndex_).setAverageLoadRate(
            averageLoadPercent->value()/100);
}


void PortWidget::on_averagePacketsPerSec_editingFinished()
{
    Q_ASSERT(plm->isPort(currentPortIndex_));

    bool isOk;
    double pps = XLocale().toDouble(averagePacketsPerSec->text(), &isOk);

    plm->port(currentPortIndex_).setAveragePacketRate(pps);
}

void PortWidget::on_averageBitsPerSec_editingFinished()
{
    Q_ASSERT(plm->isPort(currentPortIndex_));

    bool isOk;
    double bps = XLocale().toDouble(averageBitsPerSec->text(), &isOk);

    plm->port(currentPortIndex_).setAverageBitRate(bps);
}

void PortWidget::updatePortRates()
{
    if (!currentPortIndex_.isValid())
        return;

    if (!plm->isPort(currentPortIndex_))
        return;

    averageLoadPercent->setValue(
            plm->port(currentPortIndex_).averageLoadRate()*100);
    averagePacketsPerSec->setText(QString("%L1")
            .arg(plm->port(currentPortIndex_).averagePacketRate(), 0, 'f', 4));
    averageBitsPerSec->setText(QString("%L1")
            .arg(plm->port(currentPortIndex_).averageBitRate(), 0, 'f', 0));
}

void PortWidget::updatePortActions()
{
    if (!plm->isPort(currentPortIndex_))
        return;

    startTx->setEnabled(plm->port(currentPortIndex_).numStreams() > 0);
    stopTx->setEnabled(plm->port(currentPortIndex_).numStreams() > 0);
}
