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

#include "winpcapport.h"

#include "interfaceinfo.h"

#include <QCoreApplication> 
#include <QProcess> 

#ifdef Q_OS_WIN32

#include <ws2ipdef.h>

PIP_ADAPTER_ADDRESSES WinPcapPort::adapterList_ = NULL;
const uint OID_GEN_MEDIA_CONNECT_STATUS = 0x00010114;

WinPcapPort::WinPcapPort(int id, const char *device, const char *description)
    : PcapPort(id, device)
{
    populateInterfaceInfo();

    monitorRx_->stop();
    monitorTx_->stop();
    monitorRx_->wait();
    monitorTx_->wait();

    delete monitorRx_;
    delete monitorTx_;

    monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
    monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);

    data_.set_description(description);

    adapter_ = PacketOpenAdapter((CHAR*)device);
    if (!adapter_)
        qFatal("Unable to open adapter %s", device);
    linkStateOid_ = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) + 
            sizeof(uint));
    if (!linkStateOid_)
        qFatal("failed to alloc oidData");

    data_.set_is_exclusive_control(hasExclusiveControl());
    minPacketSetSize_ = 256;
}

WinPcapPort::~WinPcapPort()
{
}

OstProto::LinkState WinPcapPort::linkState()
{
    memset(linkStateOid_, 0, sizeof(PACKET_OID_DATA) + sizeof(uint));

    linkStateOid_->Oid = OID_GEN_MEDIA_CONNECT_STATUS;
    linkStateOid_->Length = sizeof(uint);

    if (PacketRequest(adapter_, 0, linkStateOid_))
    {
        uint state;

        if (linkStateOid_->Length == sizeof(state))
        {
            memcpy((void*)&state, (void*)linkStateOid_->Data, 
                    linkStateOid_->Length);
            if (state == 0)
                linkState_ = OstProto::LinkStateUp;
            else if (state == 1)
                linkState_ = OstProto::LinkStateDown;
        }
    }

    return linkState_; 
}

bool WinPcapPort::hasExclusiveControl() 
{
    QString portName(adapter_->Name + strlen("\\Device\\NPF_"));
    QString bindConfigFilePath(QCoreApplication::applicationDirPath()
                + "/bindconfig.exe");
    int exitCode;

    qDebug("%s: %s", __FUNCTION__, qPrintable(portName));

    if (!QFile::exists(bindConfigFilePath))
        return false;

    exitCode = QProcess::execute(bindConfigFilePath, 
            QStringList() << "comp" << portName);

    qDebug("%s: exit code %d", __FUNCTION__, exitCode);

    if (exitCode == 0)
        return true;
    else
        return false;
}

bool WinPcapPort::setExclusiveControl(bool exclusive) 
{
    QString portName(adapter_->Name + strlen("\\Device\\NPF_"));
    QString bindConfigFilePath(QCoreApplication::applicationDirPath()
                + "/bindconfig.exe");
    QString status;

    qDebug("%s: %s", __FUNCTION__, qPrintable(portName));

    if (!QFile::exists(bindConfigFilePath))
        return false;

    status = exclusive ? "disable" : "enable";

    QProcess::execute(bindConfigFilePath, 
            QStringList() << "comp" << portName << status);

    updateNotes(); 

    return (exclusive == hasExclusiveControl());
}

WinPcapPort::PortMonitor::PortMonitor(const char *device, Direction direction,
    AbstractPort::PortStats *stats)
    : PcapPort::PortMonitor(device, direction, stats)
{
    if (handle())
        pcap_setmode(handle(), MODE_STAT);
}

void WinPcapPort::PortMonitor::run()
{
    struct timeval lastTs;
    quint64 lastTxPkts = 0;
    quint64 lastTxBytes = 0;

    qDebug("in %s", __PRETTY_FUNCTION__);

    lastTs.tv_sec = 0;
    lastTs.tv_usec = 0;

    while (!stop_)
    {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle(), &hdr, &data);
        switch (ret)
        {
            case 1:
            {
                quint64 pkts  = *((quint64*)(data + 0));
                quint64 bytes = *((quint64*)(data + 8));

                // TODO: is it 12 or 16?
                bytes -= pkts * 12;

                uint usec = (hdr->ts.tv_sec - lastTs.tv_sec) * 1000000 + 
                    (hdr->ts.tv_usec - lastTs.tv_usec);

                switch (direction())
                {
                case kDirectionRx:
                    stats_->rxPkts += pkts;
                    stats_->rxBytes += bytes;
                    stats_->rxPps = qRound64(pkts  * 1e6 / usec);
                    stats_->rxBps = qRound64(bytes * 1e6 / usec);
                    break;

                case kDirectionTx:
                    if (isDirectional())
                    {
                        stats_->txPkts += pkts;
                        stats_->txBytes += bytes;
                    }
                    else
                    {
                        // Assuming stats_->txXXX are updated externally
                        quint64 txPkts = stats_->txPkts;
                        quint64 txBytes = stats_->txBytes;

                        pkts = txPkts - lastTxPkts;
                        bytes = txBytes - lastTxBytes;

                        lastTxPkts = txPkts;
                        lastTxBytes = txBytes;
                    }
                    stats_->txPps = qRound64(pkts  * 1e6 / usec);
                    stats_->txBps = qRound64(bytes * 1e6 / usec);
                    break;

                default:
                    Q_ASSERT(false);
                }

                break;
            }
            case 0:
                //qDebug("%s: timeout. continuing ...", __PRETTY_FUNCTION__);
                continue;
            case -1:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle()));
                break;
            case -2:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle()));
                break;
            default:
                qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
        }
        lastTs.tv_sec  = hdr->ts.tv_sec;
        lastTs.tv_usec = hdr->ts.tv_usec;
        if (!stop_)
            QThread::msleep(1000);
    }
}

void WinPcapPort::populateInterfaceInfo()
{
    if (!adapterList_) {
        qWarning("Adapter List not available");
        return;
    }

    PIP_ADAPTER_ADDRESSES adapter = adapterList_;
    while (adapter && !QString(name()).endsWith(QString(adapter->AdapterName)))
        adapter = adapter->Next;

    if (!adapter) {
        qWarning("Adapter info not found for %s", name());
        return;
    }

    interfaceInfo_ = new InterfaceInfo;
    if (adapter->PhysicalAddressLength == 6) {
        interfaceInfo_->mac = qFromBigEndian<quint64>(
                                    adapter->PhysicalAddress) >> 16;
    }
    else
        interfaceInfo_->mac = 0;

#define SOCKET_ADDRESS_FAMILY(x) \
    (x.lpSockaddr->sa_family)

#define SOCKET_ADDRESS_IP4(x) \
    (qFromBigEndian<quint32>(((sockaddr_in*)(x.lpSockaddr))->sin_addr.S_un.S_addr));

#define SOCKET_ADDRESS_IP6(x) \
    (UInt128(((PSOCKADDR_IN6)(x.lpSockaddr))->sin6_addr.u.Byte));

    // We may have multiple gateways - use the first for each family
    quint32 ip4Gateway = 0;
    PIP_ADAPTER_GATEWAY_ADDRESS gateway = adapter->FirstGatewayAddress;
    while (gateway) {
        if (SOCKET_ADDRESS_FAMILY(gateway->Address) == AF_INET) {
            ip4Gateway = SOCKET_ADDRESS_IP4(gateway->Address);
            break;
        }
        gateway = gateway->Next;
    }
    UInt128 ip6Gateway(0, 0);
    gateway = adapter->FirstGatewayAddress;
    while (gateway) {
        if (SOCKET_ADDRESS_FAMILY(gateway->Address) == AF_INET6) {
            ip6Gateway = SOCKET_ADDRESS_IP6(gateway->Address);
            break;
        }
        gateway = gateway->Next;
    }

    PIP_ADAPTER_UNICAST_ADDRESS ucast = adapter->FirstUnicastAddress;
    while (ucast) {
        if (SOCKET_ADDRESS_FAMILY(ucast->Address) == AF_INET) {
            Ip4Config ip;
            ip.address = SOCKET_ADDRESS_IP4(ucast->Address);
            ip.prefixLength = ucast->OnLinkPrefixLength;
            ip.gateway = ip4Gateway;
            interfaceInfo_->ip4.append(ip);
        }
        else if (SOCKET_ADDRESS_FAMILY(ucast->Address) == AF_INET6) {
            Ip6Config ip;
            ip.address = SOCKET_ADDRESS_IP6(ucast->Address);
            ip.prefixLength = ucast->OnLinkPrefixLength;
            ip.gateway = ip6Gateway;
            interfaceInfo_->ip6.append(ip);
        }
        ucast = ucast->Next;
    }
#undef SOCKET_ADDRESS_FAMILY
#undef SOCKET_ADDRESS_IP4
#undef SOCKET_ADDRESS_IP6
}

void WinPcapPort::fetchHostNetworkInfo()
{
    DWORD ret;
    ULONG bufLen = 15*1024; // MS recommended starting size

    while (1) {
        adapterList_ = (IP_ADAPTER_ADDRESSES *) malloc(bufLen);
        ret = GetAdaptersAddresses(AF_UNSPEC,
                                   GAA_FLAG_INCLUDE_ALL_INTERFACES
                                   | GAA_FLAG_INCLUDE_GATEWAYS,
                                   0, adapterList_, &bufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            free(adapterList_);
            continue;
        }
        break;
    }

    if (ret != NO_ERROR) {
        free(adapterList_);
        adapterList_ = NULL;
        return;
    }
}

void WinPcapPort::freeHostNetworkInfo()
{
    free(adapterList_);
    adapterList_ = NULL;
}

#endif
