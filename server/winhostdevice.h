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

#ifndef _WINDOWS_HOST_DEVICE_H
#define _WINDOWS_HOST_DEVICE_H

#include "device.h"

#ifdef Q_OS_WIN32

#include <Ws2tcpip.h>
#include <iphlpapi.h>

class WindowsHostDevice: public Device
{
public:
    WindowsHostDevice(QString portName, DeviceManager *deviceManager);
    virtual ~WindowsHostDevice() {};

    virtual void receivePacket(PacketBuffer *pktBuf);
    virtual void clearNeighbors(Device::NeighborSet set);
    virtual void getNeighbors(OstEmul::DeviceNeighborList *neighbors);

protected:
    virtual quint64 arpLookup(quint32 ip);
    virtual quint64 ndpLookup(UInt128 ip);
    virtual void sendArpRequest(quint32 tgtIp);
    virtual void sendNeighborSolicit(UInt128 tgtIp);

private:
    NET_LUID luid_;
};

#endif

#endif
