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

#ifndef _HOST_DEVICE_H
#define _HOST_DEVICE_H

#include "bsdhostdevice.h"
#include "linuxhostdevice.h"
#include "winhostdevice.h"

class DeviceManager;

/*!
 * HostDevice abstracts the various OS-specific host device classes
 */
class HostDevice
{
public:
    static Device* create(QString portName, DeviceManager *deviceManager)
    {
#if defined(Q_OS_WIN32)
        return new WindowsHostDevice(portName, deviceManager);
#elif defined(Q_OS_LINUX)
        return new LinuxHostDevice(portName, deviceManager);
#elif defined(Q_OS_BSD4)
        return new BsdHostDevice(portName, deviceManager);
#else
        (void)portName;      // squelch unused warning
        (void)deviceManager; // squelch unused warning

        return nullptr;
#endif
    }

};

#endif

