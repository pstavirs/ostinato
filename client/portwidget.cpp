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

    // XXX: pps/bps input widget is a LineEdit and not a SpinBox
    // because we want users to be able to enter values in various
    // units e.g. 1.5 Mbps, 1000K, 50 etc.

    // XXX: It's a considered decision NOT to show frame rate in
    // higher units of Kpps and Mpps as most users may not be
    // familiar with those and also we want frame rate to have a
    // high resolution for input e.g. if user enters 1,488,095.2381
    // it should NOT be shown as 1.4881 Mpps

    averagePacketsPerSec->setText(QString("%L1 pps")
            .arg(plm->port(currentPortIndex_).averagePacketRate(), 0, 'f', 4));

    double bps = plm->port(currentPortIndex_).averageBitRate();
    if (bps > 1e9)
        averageBitsPerSec->setText(tr("%L1 Gbps").arg(bps/1e9, 0, 'f', 4));
    else if (bps > 1e6)
        averageBitsPerSec->setText(tr("%L1 Mbps").arg(bps/1e6, 0, 'f', 4));
    else if (bps > 1e3)
        averageBitsPerSec->setText(tr("%L1 Kbps").arg(bps/1e3, 0, 'f', 4));
    else
        averageBitsPerSec->setText(tr("%L1 bps").arg(bps, 0, 'f', 4));

    averageLoadPercent->setValue(
            plm->port(currentPortIndex_).averageLoadRate()*100);
}

void PortWidget::updatePortActions()
{
    if (!plm->isPort(currentPortIndex_))
        return;

    startTx->setEnabled(plm->port(currentPortIndex_).numStreams() > 0);
    stopTx->setEnabled(plm->port(currentPortIndex_).numStreams() > 0);
}
