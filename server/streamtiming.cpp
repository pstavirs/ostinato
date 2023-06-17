/*
Copyright (C) 2023 Srivats P.

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

#include "streamtiming.h"

#include "timestamp.h"

#include <QCoreApplication>

StreamTiming::StreamTiming(QObject *parent)
    : QObject(parent)
{
    // This class MUST be part of the main thread so that timers can work
    Q_ASSERT(this->thread() == QCoreApplication::instance()->thread());

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &StreamTiming::processRecords);
    timer_->setInterval(3000);

    gcTimer_ = new QTimer(this);
    connect(gcTimer_, &QTimer::timeout, this, &StreamTiming::deleteStaleRecords);
    gcTimer_->setInterval(30000);
}

void StreamTiming::start(uint portId)
{
    if (activePortSet_.isEmpty()) { // First port?
        timer_->start();
        gcTimer_->start();
        qDebug("Stream Latency tracking started");
    }
    activePortSet_.insert(portId);
    qDebug("Stream Latency tracking started for port %u", portId);
}

void StreamTiming::stop(uint portId)
{
    activePortSet_.remove(portId);
    qDebug("Stream Latency tracking stopped for port %u", portId);
    if (activePortSet_.isEmpty()) { // Last port?
        processRecords();
        deleteStaleRecords();
        timer_->stop();
        gcTimer_->stop();
        qDebug("Stream Latency tracking stopped");
    }
}

quint64 StreamTiming::delay(uint portId, uint guid)
{
    Q_ASSERT(guid <= SignProtocol::kMaxGuid);

    // Process anything pending first
    processRecords();

    QMutexLocker locker(&timingLock_);

    if (!timing_.contains(portId))
        return 0;

    Timing t = timing_.value(portId)->value(guid);
    if (t.countDelays == 0)
        return 0;

    return timespecToNsecs(t.sumDelays)/t.countDelays;
}

void StreamTiming::clear(uint portId, uint guid)
{
    // XXX: We need to clear only the final timing hash; rx/tx hashes
    // are cleared by StreamTiming itself as part of processRecords and
    // deleteStaleRecords respectively
    QMutexLocker locker(&timingLock_);

    if (!timing_.contains(portId))
        return;

    PortTiming *portTiming = timing_.value(portId);
    if (!portTiming)
        return;

    if (guid == SignProtocol::kInvalidGuid)
        portTiming->clear();      // remove ALL guids
    else
        portTiming->remove(guid);
}

int StreamTiming::processRecords()
{
    // TODO: yield after a certain count of records or time when called in
    // timer context; when called from delay(), process ALL

    int count = 0;
    QMutexLocker txLocker(&txHashLock_);
    QMutexLocker rxLocker(&rxHashLock_);
    QMutexLocker timingLocker(&timingLock_);

    auto i = rxHash_.begin();
    while (i != rxHash_.end()) {
        if (txHash_.contains(i.key())) {
            struct timespec txTime = txHash_.take(i.key()).timeStamp;
            struct timespec rxTime = i.value().timeStamp;
            struct timespec diff;
            timespecsub(&rxTime, &txTime, &diff);

            uint guid = guidFromKey(i.key());
            uint portId = i.value().portId;

            if (!timing_.contains(portId))
                timing_.insert(portId, new PortTiming);
            PortTiming *portTiming = timing_.value(portId);
            Timing &guidTiming = (*portTiming)[guid];
            timespecadd(&guidTiming.sumDelays, &diff, &guidTiming.sumDelays);
            guidTiming.countDelays++;

            count++;

            timingDebug("[%u/%u/%u] diff %ld.%09ld (%ld.%09ld - %ld.%09ld)",
                i.value().portId, guid, ttagIdFromKey(i.key()),
                diff.tv_sec, diff.tv_nsec,
                rxTime.tv_sec, rxTime.tv_nsec,
                txTime.tv_sec, txTime.tv_nsec);
            timingDebug("[%u/%u](%d) total %ld.%09ld count %u",
                i.value().portId, guid, count,
                guidTiming.sumDelays.tv_sec, guidTiming.sumDelays.tv_nsec,
                guidTiming.countDelays);
        }
        i = rxHash_.erase(i);
    }

    Q_ASSERT(rxHash_.isEmpty());

    return count;
}

int StreamTiming::deleteStaleRecords()
{
    // TODO: yield after a certain count of records or time unless we are
    // idle when we process all; how do we determine we are "idle"?

    // XXX: We assume the Tx packet timestamps are based on CLOCK_REALTIME
    // (or a similar and comparable source). Since garbage collection timer
    // is not a short interval, it need not be the exact same source as long
    // as the values are comparable
    int count = 0;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    // XXX: processRecords() iterates and deletes all rx records irrespective
    // of whether it found a matching tx record. So for garbage collection we
    // only need to look at (and delete) tx records
    QMutexLocker locker(&txHashLock_);

    auto i = txHash_.begin();
    while (i != txHash_.end()) {
        struct timespec txTime = i.value().timeStamp;
        struct timespec diff;
        timespecsub(&now, &txTime, &diff);
        timingDebug("gc diff %ld", diff.tv_sec);
        if (diff.tv_sec > 30) {
            i = txHash_.erase(i);
            count++;
        } else {
            i++;
        }

    }

    if (count)
        qDebug("Latency garbage collected %d stale tx timing records", count);
    return count;
}

StreamTiming* StreamTiming::instance()
{
    static StreamTiming *instance{nullptr};

    // XXX: As of this writing, AbstractPort constructor is the first one
    // to call this - hence this singleton is created when the first port
    // is created
    if (!instance)
        instance = new StreamTiming(QCoreApplication::instance());

    return instance;
}
