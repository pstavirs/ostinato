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

StreamTiming::StreamTiming()
    : QObject(nullptr) // FIXME: parent
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &StreamTiming::processRecords);
    timer_->setInterval(3000);

    gcTimer_ = new QTimer(this);
    connect(gcTimer_, &QTimer::timeout, this, &StreamTiming::deleteStaleRecords);
    gcTimer_->setInterval(30000);
}

bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        struct timespec timestamp)
{
    Key key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    QMutexLocker locker(&txHashLock_);
    txHash_.insert(key, value);

    if (!timer_->isActive())
        timer_->start();
    return true;
}

bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        struct timespec timestamp)
{
    Key key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    QMutexLocker locker(&rxHashLock_);
    rxHash_.insert(key, value);

    if (!timer_->isActive())
        timer_->start();
    return true;
}

bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        struct timeval timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;

    return recordTxTime(portId, guid, ttagId, ts);
}

bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        struct timeval timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;
    
    return recordRxTime(portId, guid, ttagId, ts);
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
            auto timingHash = timing_[i.value().portId];
            auto guidTiming = (*timingHash)[i.key()];
            timespecadd(&guidTiming.sumDelays, &diff, &guidTiming.sumDelays);
            guidTiming.countDelays++;
            count++;
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
        if (diff.tv_sec > 30) {
            i = txHash_.erase(i);
            count++;
        }
    }

    // FIXME: when to stop gc timer?

    return count;
}

StreamTiming* StreamTiming::instance()
{
    static StreamTiming *instance{nullptr};

    if (!instance)
        instance = new StreamTiming();

    return instance;
}
