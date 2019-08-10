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

#ifndef _PCAP_SESSION_H
#define _PCAP_SESSION_H

#include <QThread>
#include <pcap.h>

#ifdef Q_OS_UNIX
#include <QHash>
#include <pthread.h>
class ThreadId {
public:
    ThreadId() {
        id_ = pthread_self();
    }
    ThreadId(pthread_t id) {
        id_ = id;
    }
    pthread_t nativeId() {
        return id_;
    }
    uint hash() const {
        QByteArray t((const char*)(&id_), sizeof(id_));
        return qHash(t);
    }
    bool operator==(const ThreadId &other) const {
        return (pthread_equal(id_, other.id_) != 0);
    }
private:
    pthread_t id_;
};

inline uint qHash(const ThreadId &key)
{
    return key.hash();
}

class PcapSession: public QThread
{
protected:
    void preRun();
    void postRun();
    void stop(pcap_t *handle);

private:
    static void signalBreakHandler(int /*signum*/);

    ThreadId thread_;
    static QHash<ThreadId, bool> signalSeen_;
};
#else
class PcapSession: public QThread
{
protected:
    void preRun() {};
    void postRun() {};
    void stop(pcap_t *handle) {
        qDebug("calling breakloop with handle %p", handle);
        pcap_breakloop(handle);
    }
};
#endif

#endif
