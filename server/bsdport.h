/*
Copyright (C) 2012 Srivats P.

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

#ifndef _SERVER_BSD_PORT_H
#define _SERVER_BSD_PORT_H

#include <QtGlobal>

#ifdef Q_OS_BSD4

#include "pcapport.h"

class BsdPort : public PcapPort
{
public:
    BsdPort(int id, const char *device);
    ~BsdPort();

    void init();

    virtual bool hasExclusiveControl();
    virtual bool setExclusiveControl(bool exclusive);

protected:
    class StatsMonitor: public QThread
    {
    public:
        StatsMonitor();
        void run();
        void stop();
        bool waitForSetupFinished(int msecs = 10000);
    private:
        static const int kRefreshFreq_ = 1; // in seconds
        bool stop_;
        bool setupDone_;
    };

    bool isPromisc_;
    bool clearPromisc_;
    static QList<BsdPort*> allPorts_;
    static StatsMonitor *monitor_; // rx/tx stats for ALL ports
};
#endif

#endif
