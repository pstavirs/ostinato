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

#ifndef _STREAM_TIMING
#define _STREAM_TIMING

#include "../common/debugdefs.h"
#include "../common/sign.h"

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QTimer>

#include <time.h>

class StreamTiming : public QObject
{
    Q_OBJECT
public:
    bool recordTxTime(uint portId, uint guid, uint ttagId,
                      const struct timespec &timestamp);
    bool recordRxTime(uint portId, uint guid, uint ttagId,
                      const struct timespec &timestamp);

    bool recordTxTime(uint portId, uint guid, uint ttagId,
                      const struct timeval &timestamp);
    bool recordRxTime(uint portId, uint guid, uint ttagId,
                      const struct timeval &timestamp);

    bool recordTxTime(uint portId, uint *ttagList, int count,
                      const struct timespec &timestamp);

    quint64 delay(uint portId, uint guid);
    void clear(uint portId, uint guid = SignProtocol::kInvalidGuid);

    static StreamTiming* instance();

public slots:
    void start(uint portId);
    void stop(uint portId);

private:
    StreamTiming(QObject *parent=nullptr);

    int processRecords();
    int deleteStaleRecords();

    quint32 makeKey(uint guid, uint ttagId) {
        return guid << 8 | (ttagId & 0xFF);
    }

    // XXX: use only time intervals, not absolute time
    quint64 timespecToNsecs(const struct timespec &interval) {
        return interval.tv_nsec + interval.tv_sec*1e9;
    }

    struct TtagData {
        struct timespec timeStamp; // nanosec resolution
        uint portId;
    };

    struct Timing {
        struct timespec sumDelays; // nanosec resolution
        uint countDelays;
    };

    QSet<uint> activePortSet_;

    // XXX: TxRxKey = guid (24 bit MSB) + ttagid (8 bit LSB)
    // TODO: encode tx port in in packet and use as part of key
    typedef quint32 TxRxKey;
    QHash<TxRxKey, TtagData> txHash_;
    QHash<TxRxKey, TtagData> rxHash_;
    QMutex txHashLock_;
    QMutex rxHashLock_;

    typedef uint PortIdKey;
    typedef uint GuidKey;
    typedef QHash<GuidKey, Timing> PortTiming;
    QHash<PortIdKey, PortTiming*> timing_;
    QMutex timingLock_;

    QTimer *timer_;     // Periodic timer to process tx/rx records
    QTimer *gcTimer_;   // Garbage collection for stale tx records
};

inline
bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        const struct timespec &timestamp)
{
    TxRxKey key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    timingDebug("[%d TX] %ld:%ld ttag %u guid %u", portId,
            timestamp.tv_sec, long(timestamp.tv_nsec), ttagId, guid);

    QMutexLocker locker(&txHashLock_);
    txHash_.insert(key, value);

    return true;
}

inline
bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        const struct timespec &timestamp)
{
    TxRxKey key = makeKey(guid, ttagId);
    TtagData value = { .timeStamp = timestamp, .portId = portId};

    timingDebug("[%d RX] %ld:%ld ttag %u guid %u", portId,
            timestamp.tv_sec, long(timestamp.tv_nsec), ttagId, guid);

    QMutexLocker locker(&rxHashLock_);
    rxHash_.insert(key, value);

    return true;
}

inline
bool StreamTiming::recordTxTime(uint portId, uint guid, uint ttagId,
        const struct timeval &timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;

    return recordTxTime(portId, guid, ttagId, ts);
}

inline
bool StreamTiming::recordRxTime(uint portId, uint guid, uint ttagId,
        const struct timeval &timestamp)
{
    struct timespec ts;
    ts.tv_sec = timestamp.tv_sec;
    ts.tv_nsec = timestamp.tv_usec*1000;

    return recordRxTime(portId, guid, ttagId, ts);
}

// TTagList contains 32-bit ttags formatted as ttagId (8msb) + guid (24lsb)
inline
bool StreamTiming::recordTxTime(uint portId, uint *ttagList, int count,
        const struct timespec &timestamp)
{
    TtagData value = { .timeStamp = timestamp, .portId = portId};
    QMutexLocker locker(&txHashLock_);

    // FIXME: Change TxRxKey to match the format passed to this function
    for (int i = 0; i < count; i++) {
        uint guid = ttagList[i] & 0x00FFFFFF;
        uint ttagId = ttagList[i] >> 24;
        TxRxKey key = makeKey(guid, ttagId);

        timingDebug("[%d TX] %ld:%ld ttag %u guid %u", portId,
                timestamp.tv_sec, long(timestamp.tv_nsec), ttagId, guid);

        txHash_.insert(key, value);
    }

    return true;
}

#endif
