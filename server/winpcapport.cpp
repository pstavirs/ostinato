/*
 *
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
#include <QTime>

#ifdef Q_OS_WIN32

#include <ifdef.h>
#include <ntddndis.h>

static const quint64 kMaxValue64 = 0xffffffffffffffffULL;
PIP_ADAPTER_ADDRESSES WinPcapPort::adapterList_ = NULL;
QList<WinPcapPort*> WinPcapPort::allPorts_;
WinPcapPort::StatsMonitor *WinPcapPort::monitor_ = NULL;
bool WinPcapPort::internalPortStats_ = false;

// FIXME: duplicated from winhostdevice - remove duplicate
static WCHAR errBuf[256];
static inline QString errStr(ulong err)
{
    return FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  errBuf, sizeof(errBuf)-1, NULL) > 0 ?
        QString("error 0x%1 %2").arg(err, 0, 16)
                          .arg(QString().fromWCharArray(errBuf)) :
        QString("error 0x%1").arg(err, 0, 16);
}

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
    monitorRx_ = monitorTx_ = NULL;

    if (internalPortStats_) {
        // One monitor each for rx and tx for each port
        monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
        monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);
    } else {
        // By default, we have one monitor for both Rx/Tx of all ports
        if (!monitor_)
            monitor_ = new StatsMonitor();
    }

    data_.set_description(description);

    // XXX: luid_ is already populated by populateInterfaceInfo() call above
    if (internalPortStats_) {
        adapter_ = PacketOpenAdapter((CHAR*)device);
        if (!adapter_)
            qFatal("Unable to open adapter %s", device);
        linkStateOid_ = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) +
                sizeof(NDIS_LINK_STATE));
        if (!linkStateOid_)
            qFatal("failed to alloc oidData");
    }

    data_.set_is_exclusive_control(hasExclusiveControl());
    minPacketSetSize_ = 256;

    qDebug("adding dev to all ports list <%s>", device);
    allPorts_.append(this);
}

WinPcapPort::~WinPcapPort()
{
    if (monitor_ && monitor_->isRunning()) {
        monitor_->stop();
        monitor_->wait();

        delete monitor_;
        monitor_ = nullptr;
    }

    // npcap will already clear promisc at exit, but we still do explicitly
    if (clearPromiscAtExit_)
        clearPromisc();
}

void WinPcapPort::init()
{
    if (monitor_) {
        if (!monitor_->isRunning())
            monitor_->start();
        monitor_->waitForSetupFinished();
    }

    if (!isPromisc_)
        addNote("Non Promiscuous Mode");

    if (monitor_)
        AbstractPort::init();
    else
        PcapPort::init();
}

OstProto::LinkState WinPcapPort::linkState()
{
    if (!internalPortStats_)
        return AbstractPort::linkState();

    assert(adapter_);
    assert(linkStateOid_);

    memset(linkStateOid_, 0, sizeof(PACKET_OID_DATA) + sizeof(NDIS_LINK_STATE));

    linkStateOid_->Oid = OID_GEN_LINK_STATE;
    linkStateOid_->Length = sizeof(NDIS_LINK_STATE);

    // TODO: migrate to the npcap-only pcap_oid_get_request() when Ostinato
    // stops supporting WinPcap
    if (PacketRequest(adapter_, 0, linkStateOid_))
    {
        uint state;

        if (linkStateOid_->Length == sizeof(NDIS_LINK_STATE))
        {
            memcpy((void*)&state,
                   (void*)(linkStateOid_->Data+sizeof(NDIS_OBJECT_HEADER)),
                   sizeof(state));
            //qDebug("%s: state = %d", data_.description().c_str(), state);
            if (state == 0)
                linkState_ = OstProto::LinkStateUnknown;
            else if (state == 1)
                linkState_ = OstProto::LinkStateUp;
            else if (state == 2)
                linkState_ = OstProto::LinkStateDown;
        }
        else {
            //qDebug("%s: link state fail", data_.description().c_str());
        }
    }
    else {
        //qDebug("%s: link state request fail", data_.description().c_str());
    }

    return linkState_;
}

bool WinPcapPort::hasExclusiveControl() 
{
    QString portName(name() + strlen("\\Device\\NPF_"));
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
    QString portName(name() + strlen("\\Device\\NPF_"));
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

bool WinPcapPort::setPromisc()
{
    if (!promiscHandle_) {
        char errbuf[PCAP_ERRBUF_SIZE] = "";

        promiscHandle_ = pcap_create(name(), errbuf);
        if (!promiscHandle_) {
            qDebug("%s: Error opening port to set promisc mode: %s",
                    name(), errbuf);
            return false;
        }

        int status = pcap_activate(promiscHandle_);
        if (status) {
            qDebug("%s: Error activating port to set promisc mode: %s",
                    name(), pcap_statustostr(status));
            pcap_close(promiscHandle_);
            promiscHandle_ = nullptr;
            return false;
        }
    }

    uint data = NDIS_PACKET_TYPE_PROMISCUOUS;
    uint size = sizeof(data);
    int status = pcap_oid_set_request(promiscHandle_,
                        OID_GEN_CURRENT_PACKET_FILTER, &data, &size);
    if (status) {
        qDebug("%s: Unable to set promisc mode: %s",
                name(), pcap_statustostr(status));
        pcap_close(promiscHandle_);
        promiscHandle_ = nullptr;
        return false;
    }

    isPromisc_ = clearPromiscAtExit_ = true;
    return true;
}

bool WinPcapPort::clearPromisc()
{
    if (!promiscHandle_) {
        qDebug("%s: No promisc handle to clear promisc mode", name());
        return false;
    }

    uint data = NDIS_PACKET_TYPE_ALL_LOCAL
                 | NDIS_PACKET_TYPE_DIRECTED
                 | NDIS_PACKET_TYPE_BROADCAST
                 | NDIS_PACKET_TYPE_MULTICAST;
    uint size = sizeof(data);
    int status = pcap_oid_set_request(promiscHandle_,
            OID_GEN_CURRENT_PACKET_FILTER, &data, &size);
    if (status) {
        qDebug("%s: Unable to clear promisc mode: %s",
                name(), pcap_statustostr(status));
        pcap_close(promiscHandle_);
        promiscHandle_ = nullptr;
        return false;
    }

    isPromisc_ = clearPromiscAtExit_ = false;
    pcap_close(promiscHandle_);

    return true;
}

#if 0
// The above set/clearPromisc implementation using standard npcap APIs.
// The below implementation uses packet.dll APIs directly and may be
// slightly more performant? To be verified!
bool WinPcapPort::setPromiscAlt(bool promisc)
{
    bool ret = false;
    uint data = promisc ? NDIS_PACKET_TYPE_PROMISCUOUS :
        (NDIS_PACKET_TYPE_ALL_LOCAL
         | NDIS_PACKET_TYPE_DIRECTED
         | NDIS_PACKET_TYPE_BROADCAST
         | NDIS_PACKET_TYPE_MULTICAST);

    PPACKET_OID_DATA oid = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) +
            sizeof(data));
    if (oid) {
        memset(oid, 0, sizeof(PACKET_OID_DATA) + sizeof(data));

        oid->Oid = OID_GEN_CURRENT_PACKET_FILTER;
        oid->Length = sizeof(data);
        memcpy(oid->Data, &data, sizeof(data));
        if (PacketRequest(adapter_, true, oid)) {
            isPromisc_ = clearPromiscAtExit_ = promisc;
            ret = true;
        } else
            qDebug("%s: Unable to %s promisc mode", name(),
                    promisc ? "set" : "clear");
    } else
        qDebug("%s: failed to alloc promisc oid", name());

    return ret;
}
#endif

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

WinPcapPort::StatsMonitor::StatsMonitor()
    : QThread()
{
    setObjectName("StatsMon");
    stop_ = false;
    setupDone_ = false;
}

WinPcapPort::StatsMonitor::~StatsMonitor()
{
}

void WinPcapPort::StatsMonitor::run()
{
    struct PortStatsAndState {
        NET_LUID luid;
        PortStats *stats;
        OstProto::LinkState *linkState;
        bool firstError;
    };
    QList<PortStatsAndState> portList;
    int count = 0;

    //
    // We first setup the port list to be polled
    //
    for (WinPcapPort* port : allPorts_) {
        if (port->luid_.Value) {
            portList.append(
                {port->luid_, &(port->stats_), &(port->linkState_), false});
            port->setPromisc();
        } else {
            qWarning("No LUID for port %s - stats will not be available",
                    port->name());
        }
        count++;
    }
    qDebug("stats port count = %d\n", count);

    if (count <= 0) {
        qWarning("No ports with valid LUID - no stats will be available");
        return;
    }
    qDebug("stats for %d ports setup", count);
    setupDone_ = true;

    //
    // We are all set - Let's start polling for stats!
    //
    while (!stop_) {
        for (auto &port : portList) {
            MIB_IF_ROW2 ifInfo;

            ifInfo.InterfaceLuid.Value = port.luid.Value;

            ulong status = GetIfEntry2(&ifInfo);
            if (status != NO_ERROR) {
                if (!port.firstError) {
                    qWarning("Failed to fetch stats for Luid 0x%016llx (%s)"
                            " - suppressing further stats error messages "
                            "for this port",
                            port.luid.Value, qPrintable(errStr(status)));
                    port.firstError = true;
                }
                continue;
            }

            switch (ifInfo.OperStatus) {
                case IfOperStatusUp:
                    *(port.linkState) = OstProto::LinkStateUp; break;
                case IfOperStatusDown:
                    *(port.linkState) = OstProto::LinkStateDown; break;
                default:
                    *(port.linkState) = OstProto::LinkStateUnknown;
            }

            quint64 inPkts = ifInfo.InUcastPkts + ifInfo.InNUcastPkts;
            port.stats->rxPps =
                ((inPkts >= port.stats->rxPkts) ?
                     inPkts - port.stats->rxPkts :
                     inPkts + (kMaxValue64 - port.stats->rxPkts))
                / kRefreshFreq_;
            port.stats->rxBps  =
                ((ifInfo.InOctets >= port.stats->rxBytes) ?
                     ifInfo.InOctets - port.stats->rxBytes :
                     ifInfo.InOctets + (kMaxValue64 - port.stats->rxBytes))
                / kRefreshFreq_;
            port.stats->rxPkts  = inPkts;
            port.stats->rxBytes = ifInfo.InOctets;

            quint64 outPkts = ifInfo.OutUcastPkts + ifInfo.OutNUcastPkts;
            port.stats->txPps  =
                ((outPkts >= port.stats->txPkts) ?
                     outPkts - port.stats->txPkts :
                     outPkts + (kMaxValue64 - port.stats->txPkts))
                / kRefreshFreq_;
            port.stats->txBps  =
                ((ifInfo.OutOctets >= port.stats->txBytes) ?
                     ifInfo.OutOctets - port.stats->txBytes :
                     ifInfo.OutOctets + (kMaxValue64 - port.stats->txBytes))
                / kRefreshFreq_;
            port.stats->txPkts  = outPkts;
            port.stats->txBytes = ifInfo.OutOctets;

            port.stats->rxDrops = ifInfo.InDiscards;
            port.stats->rxErrors = ifInfo.InErrors + ifInfo.InUnknownProtos;

            // XXX: Ostinato stats not available in Win
            //  - rxFifoErrors
            //  - rxFrameErrors
            // XXX: Win stats not available in Ostinato
            //  - OutDiscards
            //  - OutErrors
        }
        QThread::sleep(kRefreshFreq_);
    }

    portList.clear();
}

void WinPcapPort::StatsMonitor::stop()
{
    stop_ = true;
}

bool WinPcapPort::StatsMonitor::waitForSetupFinished(int msecs)
{
    QTime t;

    t.start();
    while (!setupDone_)
    {
        if (t.elapsed() > msecs)
            return false;

        QThread::msleep(10);
    }

    return true;
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
        luid_.Value = 0;
        return;
    }

    luid_ = adapter->Luid;

    interfaceInfo_ = new InterfaceInfo;

    interfaceInfo_->speed = adapter->TransmitLinkSpeed != quint64(-1) ?
        adapter->TransmitLinkSpeed/1e6 : 0;
    interfaceInfo_->mtu = adapter->Mtu;

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
