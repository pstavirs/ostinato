/*
Copyright (C) 2011 Srivats P.

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

#include "linuxport.h"

#ifdef Q_OS_LINUX

#include <QByteArray>

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

QList<LinuxPort*> LinuxPort::allPorts_;
LinuxPort::StatsMonitor *LinuxPort::monitor_;

LinuxPort::LinuxPort(int id, const char *device)
    : PcapPort(id, device) 
{
    clearPromisc_ = false;

    // We don't need per port Rx/Tx monitors for Linux
    delete monitorRx_;
    delete monitorTx_;
    monitorRx_ = monitorTx_ = NULL;

    // We have one monitor for both Rx/Tx of all ports
    if (!monitor_)
        monitor_ = new StatsMonitor();

    data_.set_is_exclusive_control(hasExclusiveControl());
    minPacketSetSize_ = 16;

    qDebug("adding dev to all ports list <%s>", device);
    allPorts_.append(this);
}

LinuxPort::~LinuxPort()
{
    qDebug("In %s", __FUNCTION__);

    if (clearPromisc_)
    {
        int sd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, name(), sizeof(ifr.ifr_name));

        if (ioctl(sd, SIOCGIFFLAGS, &ifr) != -1) 
        {
            if (ifr.ifr_flags & IFF_PROMISC)
            {
                ifr.ifr_flags &= ~IFF_PROMISC;
                if (ioctl(sd, SIOCSIFFLAGS, &ifr) == -1)
                    qDebug("Failed clearing promisc flag. SIOCSIFFLAGS failed: %s",
                        strerror(errno));
            }
        }
        else
            qDebug("Failed clearing promisc flag. SIOCGIFFLAGS failed: %s",
                    strerror(errno));

        close(sd);
    }
}

void LinuxPort::init()
{
    // TODO: Update Notes with Promisc/Non-Promisc

    if (!monitor_->isRunning())
        monitor_->start();
}

OstProto::LinkState LinuxPort::linkState()
{
    // TODO
    return linkState_; 
}

bool LinuxPort::hasExclusiveControl() 
{
    // TODO
    return false;
}

bool LinuxPort::setExclusiveControl(bool /*exclusive*/) 
{
    // TODO
    return false;
}

LinuxPort::StatsMonitor::StatsMonitor()
    : QThread()
{
}

void LinuxPort::StatsMonitor::run()
{
    PortStats **portStats;
    int fd, sd;
    QByteArray buf;
    int len;
    char *p, *end;
    int count, index;
    const char* fmtopt[] = {
        "%llu%llu%u%u%u%u%u%u%llu%llu%u%u%u%u%u%u\n",
        "%llu%llu%u%u%u%u%n%n%llu%llu%u%u%u%u%u%n\n",
    };
    const char *fmt;
    struct ifreq ifr;

    //
    // We first setup stuff before we start polling for stats
    //
    fd = open("/proc/net/dev", O_RDONLY);
    if (fd < 0)
    {
        qWarning("Unable to open /proc/net/dev - no stats will be available");
        return;
    }

    buf.fill('\0', 8192);
    len = read(fd, (void*) buf.data(), buf.size());
    if (len < 0)
    {
        qWarning("initial buffer size is too small. no stats will be available");
        return;
    }

    p = buf.data();
    end = p + len;

    // Select scanf format
    if (strstr(buf, "compressed"))
        fmt = fmtopt[0];
    else 
        fmt = fmtopt[1];

    // Count number of lines - number of ports is 2 less than number of lines
    count = 0;
    while (p < end)
    {
        if (*p == '\n')
            count++;
        p++;
    }
    count -= 2;

    if (count <= 0)
    {
        qWarning("no ports in /proc/dev/net - no stats will be available");
        return;
    }

    portStats = (PortStats**) calloc(count, sizeof(PortStats));
    Q_ASSERT(portStats != NULL);

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    Q_ASSERT(sd >= 0);
    memset(&ifr, 0, sizeof(ifr));

    //
    // Populate the port stats array
    //
    p = buf.data();

    // Skip first two lines
    while (*p != '\n')
        p++;
    p++;
    while (*p != '\n')
        p++;
    p++;

    index = 0;
    while (p < end)
    {
        char* q;

        // Skip whitespace
        while ((p < end) && (*p == ' '))
            p++;

        q = p;

        // Get interface name
        while ((q < end) && (*q != ':') && (*q != '\n'))
            q++;

        if ((q < end) && (*q == ':'))
        {
            foreach(LinuxPort* port, allPorts_)
            {
                if (strncmp(port->name(), p, int(q-p)) == 0)
                {
                    portStats[index] = &(port->stats_);

                    // Set promisc mode, if not already set
                    strncpy(ifr.ifr_name, port->name(), sizeof(ifr.ifr_name));
                    if (ioctl(sd, SIOCGIFFLAGS, &ifr) != -1)
                    {
                        if ((ifr.ifr_flags & IFF_PROMISC) == 0)
                        {
                            ifr.ifr_flags |= IFF_PROMISC;
                            if (ioctl(sd, SIOCSIFFLAGS, &ifr) != -1)
                                port->clearPromisc_ = true;
                            else
                            {
                                qDebug("%s: failed to set promisc; "
                                        "SIOCSIFFLAGS failed (%s)", 
                                        port->name(), strerror(errno));
                            }
                        }
                    }
                    else
                    {
                        qDebug("%s: failed to set promisc; SIOCGIFFLAGS failed (%s)",
                                port->name(), strerror(errno));
                    }
                    break;
                }
            }
        }
        index++;

        // Skip till newline
        p = q;
        while (*p != '\n')
            p++;
        p++;
    }
    Q_ASSERT(index == count);

    close(sd);

    qDebug("stats for %d ports setup", count);

    //
    // We are all set - Let's start polling for stats!
    //
    while (1)
    {
        lseek(fd, 0, SEEK_SET);
        len = read(fd, (void*) buf.data(), buf.size());
        if (len < 0)
        {
            if (buf.size() > 1*1024*1024)
            {
                qWarning("buffer size hit limit. no more stats");
                return;
            }
            qDebug("doubling buffer size. curr = %d", buf.size());
            buf.resize(buf.size() * 2);
            continue;
        }

        p = buf.data();
        end = p + len;

        // Skip first two lines
        while (*p != '\n')
            p++;
        p++;
        while (*p != '\n')
            p++;
        p++;

        index = 0;
        while (p < end)
        {
            uint dummy;
            quint64 rxBytes, rxPkts;
            quint64 txBytes, txPkts;

            // Skip interface name - we assume the number and order of ports
            // won't change since we parsed the output before we started polling
            while ((p < end) && (*p != ':') && (*p != '\n'))
                p++;
            if (p >= end)
                break;
            if (*p == '\n')
            {
                index++;
                continue;
            }
            p++;

            sscanf(p, fmt,
                    &rxBytes, &rxPkts, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
                    &txBytes, &txPkts, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);

            if (index < count)
            {
                AbstractPort::PortStats *stats = portStats[index];
                if (stats)
                {
                    stats->rxPps  = (rxPkts - stats->rxPkts)/kRefreshFreq_;
                    stats->rxBps  = (rxBytes - stats->rxBytes)/kRefreshFreq_;
                    stats->rxPkts  = rxPkts;
                    stats->rxBytes = rxBytes;
                    stats->txPps  = (txPkts - stats->txPkts)/kRefreshFreq_;
                    stats->txBps  = (txBytes - stats->txBytes)/kRefreshFreq_;
                    stats->txPkts  = txPkts;
                    stats->txBytes = txBytes;
                }
            }

            while (*p != '\n')
                p++;
            p++;
            index++;
        }
        QThread::sleep(kRefreshFreq_);
    }
}

#endif
