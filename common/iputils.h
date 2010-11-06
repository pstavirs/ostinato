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

quint32 ipAddress(quint32 baseIp, int prefix, AddrMode mode, int count, 
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
        qWarning("Unhandled mode = %d", mode);
    }

    return ip;
}

} // namespace ipUtils
#endif
