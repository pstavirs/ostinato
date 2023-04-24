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

StreamTiming::StreamTiming()
    : QObject(nullptr) // FIXME: parent
{
    // This class must be part of the main thread so that timers can work
    Q_ASSERT(this->thread() == QCoreApplication::instance()->thread());

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &StreamTiming::processRecords);
    timer_->setInterval(3000);
    timer_->start();

    gcTimer_ = new QTimer(this);
    connect(gcTimer_, &QTimer::timeout, this, &StreamTiming::deleteStaleRecords);
    gcTimer_->setInterval(30000);
    gcTimer_->start();
}

bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        const struct timespec &timestamp)
{
    TxRxKey key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    QMutexLocker locker(&txHashLock_);
    txHash_.insert(key, value);

    return true;
}

bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        const struct timespec &timestamp)
{
    TxRxKey key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    QMutexLocker locker(&rxHashLock_);
    rxHash_.insert(key, value);

    return true;
}

bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        const struct timeval &timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;

    return recordTxTime(portId, guid, ttagId, ts);
}

bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        const struct timeval &timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;
    
    return recordRxTime(portId, guid, ttagId, ts);
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

    qDebug("XXXX [%u/%u] %lldns", portId, guid,
        timespecToNsecs(t.sumDelays)/t.countDelays);

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
    // FIXME: yield after a certain count of records or time?

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

            uint guid = i.key() >> 8;
            uint portId = i.value().portId;

            if (!timing_.contains(portId))
                timing_.insert(portId, new PortTiming);
            PortTiming *portTiming = timing_.value(portId);
            Timing &guidTiming = (*portTiming)[guid];
            timespecadd(&guidTiming.sumDelays, &diff, &guidTiming.sumDelays);
            guidTiming.countDelays++;

            count++;

            qDebug("XXXXX [%u/%u/%u] diff %ld.%09ld (%ld.%09ld - %ld.%09ld)",
                i.value().portId, guid, i.key() & 0xFF,
                diff.tv_sec, diff.tv_nsec,
                rxTime.tv_sec, rxTime.tv_nsec,
                txTime.tv_sec, txTime.tv_nsec);
            qDebug("XXXXX %d:[%u/%u] total %ld.%09ld count %u",
                count, i.value().portId, guid,
                guidTiming.sumDelays.tv_sec, guidTiming.sumDelays.tv_nsec,
                guidTiming.countDelays);
        }
        i = rxHash_.erase(i);
    }

    Q_ASSERT(rxHash_.isEmpty());

    // FIXME: when to stop timer?

    return count;
}

int StreamTiming::deleteStaleRecords()
{
    // FIXME: yield after a certain count of records or time?

    // FIXME: Can we compare 'now' below with libpcap provided timestamp?
    // Are their sources same or synced?
    int count = 0;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // XXX: processRecords() iterates and deletes all rx records irrespective
    // of whether it found a matching tx record. So for garbage collection we
    // only need to look at (and delete) tx records
    QMutexLocker locker(&txHashLock_);

    auto i = txHash_.begin();
    while (i != txHash_.end()) {
        struct timespec txTime = i.value().timeStamp;
        struct timespec diff;
        timespecsub(&now, &txTime, &diff);
        qDebug("XXXX gc diff %ld", diff.tv_sec);
        if (diff.tv_sec > 30) {
            i = txHash_.erase(i);
            count++;
            qDebug("XXXX -%d", count);
        } else {
            i++;
        }

    }

    // FIXME: when to stop gc timer?

    qDebug("XXXX garbage collected %d stale tx timing records", count);
    return count;
}

StreamTiming* StreamTiming::instance()
{
    static StreamTiming *instance{nullptr};

    if (!instance)
        instance = new StreamTiming();

    return instance;
}
