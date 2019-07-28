/*
Copyright (C) 2010-2012 Srivats P.

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

#include "portmanager.h"

#include "bsdport.h"
#include "interfaceinfo.h"
#include "linuxport.h"
#include "pcapport.h"
#include "settings.h"
#include "winpcapport.h"

#include <QHostAddress>
#include <QStringList>

PortManager *PortManager::instance_ = NULL;

#if defined(Q_OS_WIN32)
#include <QUuid>
#include <ipHlpApi.h>
// Define the function prototypes since they are not defined in ipHlpApi.h
NETIO_STATUS WINAPI ConvertInterfaceGuidToLuid(
        const GUID *InterfaceGuid, PNET_LUID InterfaceLuid);
NETIO_STATUS WINAPI ConvertInterfaceLuidToAlias(
        const NET_LUID *InterfaceLuid, PWSTR InterfaceAlias, SIZE_T Length);

#define MyGetProcAddress(hDll, function) \
    hDll ? reinterpret_cast<decltype(&function)> (GetProcAddress(hDll, #function)) : NULL;
#endif

PortManager::PortManager()
{
    int i;
    pcap_if_t *device;
    AbstractPort::Accuracy txRateAccuracy;

    qDebug("PCAP Lib: %s", pcap_lib_version());
    qDebug("Retrieving the device list from the local machine\n"); 

#if defined(Q_OS_WIN32)
    WinPcapPort::fetchHostNetworkInfo();
#elif defined(Q_OS_LINUX)
    LinuxPort::fetchHostNetworkInfo();
#elif defined(Q_OS_BSD4)
    BsdPort::fetchHostNetworkInfo();
#endif

    txRateAccuracy = rateAccuracy();

    pcap_if_t *deviceList = GetPortList();

    for(device = deviceList, i = 0; device != NULL; device = device->next, i++)
    {
        AbstractPort *port;
      
        qDebug("%d. %s", i, device->name);
        if (device->description)
            qDebug(" (%s)\n", device->description);

#if defined(Q_OS_WIN32)
        if (!filterAcceptsPort(device->description))
#else
        if (!filterAcceptsPort(device->name))
#endif
        {
            qDebug("%s (%s) rejected by filter. Skipping!",
                    device->name, device->description);
            i--;
            continue;
        }

#if defined(Q_OS_WIN32)
        port = new WinPcapPort(i, device->name, device->description);
#elif defined(Q_OS_LINUX)
        port = new LinuxPort(i, device->name);
#elif defined(Q_OS_BSD4)
        port = new BsdPort(i, device->name);
#else
        port = new PcapPort(i, device->name);
#endif

        if (!port->isUsable())
        {
            qDebug("%s: unable to open %s. Skipping!", __FUNCTION__,
                    device->name);
            delete port;
            i--;
            continue;
        }

        const InterfaceInfo *intfInfo = port->interfaceInfo();
        if (intfInfo) {
            qDebug("Mac: %012llx", intfInfo->mac);
            foreach(Ip4Config ip, intfInfo->ip4)
                qDebug("Ip4: %s/%d gw: %s",
                        qPrintable(QHostAddress(ip.address).toString()),
                        ip.prefixLength,
                        qPrintable(QHostAddress(ip.gateway).toString()));
            foreach(Ip6Config ip, intfInfo->ip6)
                qDebug("Ip6: %s/%d gw: %s",
                        qPrintable(QHostAddress(ip.address.toArray()).toString()),
                        ip.prefixLength,
                        qPrintable(QHostAddress(ip.gateway.toArray()).toString()));
        }

        if (!port->setRateAccuracy(txRateAccuracy))
            qWarning("failed to set rateAccuracy (%d)", txRateAccuracy);

        portList_.append(port);
    }

    FreePortList(deviceList);

    foreach(AbstractPort *port, portList_)
        port->init();
    
#if defined(Q_OS_WIN32)
    WinPcapPort::freeHostNetworkInfo();
#elif defined(Q_OS_LINUX)
    LinuxPort::freeHostNetworkInfo();
#elif defined(Q_OS_BSD4)
    BsdPort::freeHostNetworkInfo();
#endif

    return;
}

PortManager::~PortManager()
{
    while (!portList_.isEmpty())
        delete portList_.takeFirst();
}

PortManager* PortManager::instance()
{
    if (!instance_)
        instance_ = new PortManager;

    return instance_;       
}

pcap_if_t* PortManager::GetPortList()
{
    pcap_if_t *deviceList = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&deviceList, errbuf) == -1)
        qDebug("Error in pcap_findalldevs_ex: %s\n", errbuf);

#if defined(Q_OS_WIN32)
    // Use windows' connection name as the description for a better UX
    ipHlpApi_ = LoadLibrary(TEXT("ipHlpApi.dll"));
    auto guid2Luid = MyGetProcAddress(ipHlpApi_, ConvertInterfaceGuidToLuid);
    auto luid2Alias = MyGetProcAddress(ipHlpApi_, ConvertInterfaceLuidToAlias);

    if (guid2Luid && luid2Alias) {
        pcap_if_t *device;
        for(device = deviceList; device != NULL; device = device->next) {
            GUID guid = static_cast<GUID>(QUuid(
                            QString(device->name).remove("\\Device\\NPF_")));
            NET_LUID luid;

            oldDescriptions_.append(device->description);
            newDescriptions_.append(new QByteArray());
            if (guid2Luid(&guid, &luid) == NO_ERROR) {
                WCHAR conn[256];
                if (luid2Alias(&luid, conn, 256) == NO_ERROR) {
                    *(newDescriptions_.last()) = QString().fromWCharArray(conn)
                                                            .toLocal8Bit();
                    device->description = newDescriptions_.last()->data();
                }
            }
        }
    }
#endif

    return deviceList;
}

void PortManager::FreePortList(pcap_if_t *deviceList)
{
#if defined(Q_OS_WIN32)
    int i = 0;
    pcap_if_t *device;
    if (oldDescriptions_.size()) {
        for(device = deviceList; device != NULL; device = device->next)
            device->description = oldDescriptions_.at(i++);
    }
    oldDescriptions_.clear();

    while (newDescriptions_.size())
        delete newDescriptions_.takeFirst();

    if (ipHlpApi_)
        FreeLibrary(ipHlpApi_);
#endif

    pcap_freealldevs(deviceList);
}

AbstractPort::Accuracy PortManager::rateAccuracy()
{
    QString rateAccuracy = appSettings->value(kRateAccuracyKey, 
                                  kRateAccuracyDefaultValue).toString();
    if (rateAccuracy == "High")
        return AbstractPort::kHighAccuracy;
    else if (rateAccuracy == "Low")
        return AbstractPort::kLowAccuracy;
    else
        qWarning("Unsupported RateAccuracy setting - %s", 
                 qPrintable(rateAccuracy));

    return AbstractPort::kHighAccuracy;
}

bool PortManager::filterAcceptsPort(const char *name)
{
    QRegExp pattern;
    QStringList includeList = appSettings->value(kPortListIncludeKey)
                                    .toStringList();
    QStringList excludeList = appSettings->value(kPortListExcludeKey)
                                    .toStringList();

    pattern.setPatternSyntax(QRegExp::Wildcard);

    // An empty (or missing) includeList accepts all ports
    // NOTE: A blank "IncludeList=" is read as a stringlist with one
    // string which is empty => treat it same as an empty stringlist
    if (includeList.isEmpty()
            || (includeList.size() == 1 && includeList.at(0).isEmpty()))
        goto _include_pass;

    foreach (QString str, includeList) {
        pattern.setPattern(str);
        if (pattern.exactMatch(name))
            goto _include_pass;
    }

    // IncludeList is not empty and port did not match a pattern
    return false;

_include_pass:
    foreach (QString str, excludeList) {
        pattern.setPattern(str);
        if (pattern.exactMatch(name))
            return false;
    }

    // Port did not match a pattern in ExcludeList
    return true;
}
