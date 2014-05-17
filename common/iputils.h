/*
Copyright (C) 2010 Srivats P.

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

#ifndef _IP_UTILS_H
#define _IP_UTILS_H

namespace ipUtils {
enum AddrMode {
    kFixed = 0,
    kIncrement = 1,
    kDecrement = 2,
    kRandom = 3
};

quint32 inline ipAddress(quint32 baseIp, int prefix, AddrMode mode, int count, 
                    int index)
{
    int u;
    quint32 mask = ((1<<prefix) - 1) << (32 - prefix);
    quint32 subnet, host, ip;

    switch(mode)
    {
    case kFixed:
        ip = baseIp;
        break;
    case kIncrement:
        u = index % count;
        subnet = baseIp & mask;
        host = (((baseIp & ~mask) + u) &
            ~mask);
        ip = subnet | host;
        break;
    case kDecrement:
        u = index % count;
        subnet = baseIp & mask;
        host = (((baseIp & ~mask) - u) &
            ~mask);
        ip = subnet | host;
        break;
    case kRandom:
        subnet = baseIp & mask;
        host = (qrand() & ~mask);
        ip = subnet | host;
        break;
    default:
        ip = 0;
        qWarning("Unhandled mode = %d", mode);
    }

    return ip;
}

void inline ipAddress(quint64 baseIpHi, quint64 baseIpLo, int prefix, 
        AddrMode mode, int count, int index, quint64 &ipHi, quint64 &ipLo)
{
    int u, p, q;
    quint64 maskHi = 0, maskLo = 0;
    quint64 prefixHi, prefixLo;
    quint64 hostHi = 0, hostLo = 0;

    switch(mode)
    {
        case kFixed:
            ipHi = baseIpHi;
            ipLo = baseIpLo;
            break;
        case kIncrement:
        case kDecrement:
        case kRandom:
            u = index % count;
            if (prefix > 64) {
                p = 64;
                q = prefix - 64;
            } else {
                p = prefix;
                q = 0;
            }
            if (p > 0) 
                maskHi = ~((quint64(1) << p) - 1);
            if (q > 0) 
                maskLo = ~((quint64(1) << q) - 1);
            prefixHi = baseIpHi & maskHi;
            prefixLo = baseIpLo & maskLo;
            if (mode == kIncrement) {
                hostHi = ((baseIpHi & ~maskHi) + 0) & ~maskHi;
                hostLo = ((baseIpLo & ~maskLo) + u) & ~maskLo;
            } 
            else if (mode == kDecrement) {
                hostHi = ((baseIpHi & ~maskHi) - 0) & ~maskHi;
                hostLo = ((baseIpLo & ~maskLo) - u) & ~maskLo;
            } 
            else if (mode==kRandom) {
                hostHi = qrand() & ~maskHi;
                hostLo = qrand() & ~maskLo;
            }
            ipHi = prefixHi | hostHi;
            ipLo = prefixLo | hostLo;
            break;
        default:
            qWarning("Unhandled mode = %d", mode);
    }
}

} // namespace ipUtils
#endif
