/*
Copyright (C) 2018 Srivats P.

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

#ifndef _INTERFACE_INFO_H
#define _INTERFACE_INFO_H

#include "../common/uint128.h"
#include <QtGlobal>

template <typename IpType>
struct IpConfig
{
    IpType  address;
    int     prefixLength;
    IpType  gateway;
};

using Ip4Config = IpConfig<quint32>;
using Ip6Config = IpConfig<UInt128>;

struct InterfaceInfo
{
    quint64          mac;
    QList<Ip4Config> ip4;
    QList<Ip6Config> ip6;
};

#endif
