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

#include <QHash>
#include <QMutex>
#include <QTimer>

#include <time.h>

class StreamTiming : public QObject
{
    Q_OBJECT
public:
    StreamTiming(int portCount);

    bool recordTxTime(uint portId, uint guid, uint ttagId,
                      struct timespec timestamp);
    bool recordRxTime(uint portId, uint guid, uint ttagId,
                      struct timespec timestamp);

    bool recordTxTime(uint portId, uint guid, uint ttagId,
                      struct timeval timestamp);
    bool recordRxTime(uint portId, uint guid, uint ttagId,
                      struct timeval timestamp);

    static StreamTiming* instance();

private:
    int processRecords();
    int deleteStaleRecords();

    quint32 makeKey(uint guid, uint ttagId) {
        return guid << 24 | (ttagId & 0xFF);
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


    // XXX: Key = guid (24 bit MSG), ttagid (8 bit LSB)
    // TODO: encode tx port in in packet and use as part of key
    typedef quint32 Key;
    QHash<Key, TtagData> txHash_;
    QHash<Key, TtagData> rxHash_;
    QList<QHash<Key, Timing>*> timing_; // list index => portId
    QMutex txHashLock_;
    QMutex rxHashLock_;
    QMutex timingLock_; // FIXME: change to RW lock?

    QTimer *timer_;     // Periodic timer to process tx/rx records
    QTimer *gcTimer_;   // Garbage collection for stale tx records
};

#endif
