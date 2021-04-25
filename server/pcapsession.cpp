/*
Copyright (C) 2019 Srivats P.

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

#include "pcapsession.h"

#ifdef Q_OS_UNIX
#include <signal.h>
#include <typeinfo>

#define MY_BREAK_SIGNAL SIGUSR1

QHash<ThreadId, bool> PcapSession::signalSeen_;

void PcapSession::preRun()
{
    // Should be called in the thread's context
    thread_ = pthread_self();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = PcapSession::signalBreakHandler;
    if (!sigaction(MY_BREAK_SIGNAL, &sa, NULL)) {
        signalSeen_[thread_] = false;
        qDebug("Break signal handler installed");
    }
    else
        qWarning("Failed to install MY_BREAK_SIGNAL handler");
}

void PcapSession::postRun()
{
    // Should be called in the thread's context
    ThreadId id = pthread_self();
    qDebug("In %s::%s", typeid(*this).name(), __FUNCTION__);
    if (!signalSeen_.contains(id)) {
        qWarning("Thread not found in signalSeen");
        return;
    }

    bool &seen = signalSeen_[id];
    // XXX: don't exit the thread until we see the signal; if we don't
    // some platforms will crash
    if (!seen) {
        qDebug("Wait for signal");
        while (!seen)
            QThread::msleep(10);
    }
    signalSeen_.remove(id);
    qDebug("Signal seen and handled");
}

void PcapSession::stop(pcap_t *handle)
{
    // Should be called OUTSIDE the thread's context
    // XXX: As per the man page for pcap_breakloop, we need both
    // pcap_breakloop and a mechanism to interrupt system calls;
    // we use a signal for the latter
    // TODO: If the signal mechanism doesn't work, we could try
    // pthread_cancel(thread_);
    pcap_breakloop(handle);
    pthread_kill(thread_.nativeId(), MY_BREAK_SIGNAL);
}

void PcapSession::signalBreakHandler(int /*signum*/)
{
    qDebug("In %s", __FUNCTION__);
    signalSeen_[pthread_self()] = true;
}
#endif
