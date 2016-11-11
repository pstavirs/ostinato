/*
Copyright (C) 2016 Srivats P.

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

#include "pcaptxstats.h"

#include "pcaptxstats.h"
#include "statstuple.h"

PcapTxStats::PcapTxStats()
{
    txThreadStats_ = NULL;

    stats_ = new AbstractPort::PortStats;
    usingInternalStats_ = true;

    stop_ = false;
}

PcapTxStats::~PcapTxStats()
{
    if (usingInternalStats_)
        delete stats_;
}

void PcapTxStats::setTxThreadStats(StatsTuple *stats)
{
    txThreadStats_ = stats;
}

void PcapTxStats::useExternalStats(AbstractPort::PortStats *stats)
{
    if (usingInternalStats_)
        delete stats_;
    stats_ = stats;
    usingInternalStats_ = false;
}

void PcapTxStats::start()
{
    QThread::start();

    while (!isRunning())
        QThread::msleep(10);
}

void PcapTxStats::stop()
{
    stop_ = true;

    while (isRunning())
        QThread::msleep(10);
}

void PcapTxStats::run()
{
    Q_ASSERT(txThreadStats_);

    qDebug("txStats: collection start");

    while (1) {
        stats_->txPkts = txThreadStats_->pkts;
        stats_->txBytes = txThreadStats_->bytes;

        if (stop_)
            break;
        QThread::msleep(1000);
    }
    stop_ = false;
    qDebug("txStats: collection end");
}
