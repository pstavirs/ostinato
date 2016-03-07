/*
Copyright (C) 2016 Srivats P.

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

#ifndef _EMULATION_H
#define _EMULATION_H

#include "emulproto.pb.h"

#include <QtGlobal>

static inline OstProto::DeviceGroup* newDeviceGroup(uint portId)
{
    OstProto::DeviceGroup *devGrp = new OstProto::DeviceGroup;

    // To ensure that DeviceGroup have a unique key, we assign
    // a random mac address upon creation; ideally, it would
    // have been good to inherit OstProto::DeviceGroup and define
    // a new constructor, but ProtoBuf forbids against inheriting
    // generated classes, so we use this helper function instead
    //
    // Create a mac address as per RFC 4814 Sec 4.2
    // (RR & 0xFC):PP:PP:RR:RR:RR
    // where RR is a random number, PP:PP is 1-indexed port index
    // NOTE: although qrand() return type is a int, the max value
    // is RAND_MAX (stdlib.h) which is often 16-bit only, so we
    // use two random numbers
    quint32 r1 = qrand(), r2 = qrand();
    quint64 mac;
    mac = quint64(r1 & 0xfc00) << 32
        | quint64(portId + 1) << 24
        | quint64((r1 & 0xff) << 16 | (r2 & 0xffff));
    devGrp->MutableExtension(OstEmul::mac)->set_address(mac);

    return devGrp;
}


#endif

