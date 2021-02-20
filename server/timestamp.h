/*
Copyright (C) 2010-2016 Srivats P.

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

#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

#include "timespecops.h"

#include <QtGlobal>

#if defined(Q_OS_LINUX)
#include <sys/time.h>
#ifdef USE_NSEC_TIMESTAMP
typedef struct timespec TimeStamp;
static void inline getTimeStamp(TimeStamp *stamp)
{
    clock_gettime(CLOCK_MONOTONIC, stamp);
}

// Returns time diff in nsecs between end and start
static long inline ndiffTimeStamp(const TimeStamp *start, const TimeStamp *end)
{
    struct timespec diff;
    timespecsub(end, start, &diff);

    long nsecs = diff.tv_nsec;
    if (diff.tv_sec)
        nsecs += diff.tv_sec*1e9;

    return nsecs;
}
#else
typedef struct timeval TimeStamp;
static void inline getTimeStamp(TimeStamp *stamp)
{
    gettimeofday(stamp, NULL);
}

// Returns time diff in usecs between end and start
static long inline udiffTimeStamp(const TimeStamp *start, const TimeStamp *end)
{
    struct timeval diff;
    long usecs;

    timersub(end, start, &diff);

    usecs = diff.tv_usec;
    if (diff.tv_sec)
        usecs += diff.tv_sec*1e6;

    return usecs;
}
#endif

#elif defined(Q_OS_WIN32)
static quint64 gTicksFreq;
typedef LARGE_INTEGER TimeStamp;
static void inline getTimeStamp(TimeStamp* stamp)
{
    QueryPerformanceCounter(stamp);
}

static long inline udiffTimeStamp(const TimeStamp *start, const TimeStamp *end)
{
    if (end->QuadPart >= start->QuadPart)
        return (end->QuadPart - start->QuadPart)*long(1e6)/gTicksFreq;
    else
    {
        // FIXME: incorrect! what's the max value for this counter before
        // it rolls over?
        return (start->QuadPart)*long(1e6)/gTicksFreq;
    }
}
#else
typedef int TimeStamp;
static void inline getTimeStamp(TimeStamp*) {}
static long inline udiffTimeStamp(const TimeStamp*, const TimeStamp*) { return 0; }
#endif

#endif

