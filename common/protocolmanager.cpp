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

#include "protocolmanager.h"
#include "abstractprotocol.h"

#include "protocol.pb.h"
#if 0
#include "textproto.h"
#include "userscript.h"
#include "hexdump.h"
#include "sample.h"
#else
#include "mac.h"
#include "payload.h"
#include "vlan.h"
#include "svlan.h"
#include "vlanstack.h"    
#include "dot3.h"    
#include "llc.h"    
#include "dot2llc.h"
#include "snap.h"    
#include "dot2snap.h"
#include "eth2.h"    
// L3 Protos
#include "arp.h"    
#include "ip4.h"    
#include "ip6.h"
#include "ip4over4.h"    
#include "ip4over6.h"    
#include "ip6over4.h"    
#include "ip6over6.h"    
// L4 Protos
#include "icmp.h"    
#include "igmp.h"    
#include "mld.h"    
#include "tcp.h"    
#include "udp.h"    
#endif

ProtocolManager *OstProtocolManager;

ProtocolManager::ProtocolManager()
{
    /*! \todo (LOW) calls to registerProtocol() should be done by the protocols
     themselves (once this is done remove the #includes for all the protocols)
     */
#if 0
    registerProtocol(OstProto::Protocol::kTextProtocolFieldNumber,
            (void*) TextProtocol::createInstance);

    registerProtocol(OstProto::Protocol::kHexDumpFieldNumber,
            (void*) HexDumpProtocol::createInstance);

    registerProtocol(OstProto::Protocol::kUserScriptFieldNumber,
            (void*) UserScriptProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSampleFieldNumber,
            (void*) SampleProtocol::createInstance);
#else
    registerProtocol(OstProto::Protocol::kMacFieldNumber,
            (void*) MacProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kPayloadFieldNumber,
            (void*) PayloadProtocol::createInstance);

    registerProtocol(OstProto::Protocol::kVlanFieldNumber,
            (void*) VlanProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSvlanFieldNumber,
            (void*) SVlanProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kVlanStackFieldNumber,
            (void*) VlanStackProtocol::createInstance);

    registerProtocol(OstProto::Protocol::kEth2FieldNumber,
            (void*) Eth2Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot3FieldNumber,
            (void*) Dot3Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kLlcFieldNumber,
            (void*) LlcProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot2LlcFieldNumber,
            (void*) Dot2LlcProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSnapFieldNumber,
            (void*) SnapProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot2SnapFieldNumber,
            (void*) Dot2SnapProtocol::createInstance);

    // Layer 3 Protocols
    registerProtocol(OstProto::Protocol::kArpFieldNumber,
            (void*) ArpProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp4FieldNumber,
            (void*) Ip4Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp6FieldNumber,
            (void*) Ip6Protocol::createInstance);

    registerProtocol(OstProto::Protocol::kIp4over4FieldNumber,
            (void*) Ip4over4Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp4over6FieldNumber,
            (void*) Ip4over6Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp6over4FieldNumber,
            (void*) Ip6over4Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp6over6FieldNumber,
            (void*) Ip6over6Protocol::createInstance);

    // Layer 4 Protocols
    registerProtocol(OstProto::Protocol::kIcmpFieldNumber,
            (void*) IcmpProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kIgmpFieldNumber,
            (void*) IgmpProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kMldFieldNumber,
            (void*) MldProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kTcpFieldNumber,
            (void*) TcpProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kUdpFieldNumber,
            (void*) UdpProtocol::createInstance);
#endif
    populateNeighbourProtocols();
}

ProtocolManager::~ProtocolManager()
{
    numberToNameMap.clear();
    nameToNumberMap.clear();
    neighbourProtocols.clear();
    factory.clear();
    QList<AbstractProtocol*> pl = protocolList.values();
    while (!pl.isEmpty())
        delete pl.takeFirst();
}

void ProtocolManager::registerProtocol(int protoNumber,
    void *protoInstanceCreator)
{
    AbstractProtocol *p;

    Q_ASSERT(!factory.contains(protoNumber));

    factory.insert(protoNumber, protoInstanceCreator);

    p = createProtocol(protoNumber, NULL);
    protocolList.insert(protoNumber, p);

    numberToNameMap.insert(protoNumber, p->shortName());
    nameToNumberMap.insert(p->shortName(), protoNumber);
}

void ProtocolManager::populateNeighbourProtocols()
{
    neighbourProtocols.clear();

    foreach(AbstractProtocol *p, protocolList)
    {
        if (p->protocolIdType() != AbstractProtocol::ProtocolIdNone)
        {
            foreach(AbstractProtocol *q, protocolList)
            {
                if (q->protocolId(p->protocolIdType()))
                    neighbourProtocols.insert(
                        p->protocolNumber(), q->protocolNumber());
            }
        }
    }
}

bool ProtocolManager::isRegisteredProtocol(int protoNumber)
{
    return factory.contains(protoNumber);
}

AbstractProtocol* ProtocolManager::createProtocol(int protoNumber,
    StreamBase *stream, AbstractProtocol *parent)
{
    AbstractProtocol* (*pc)(StreamBase*, AbstractProtocol*);
    AbstractProtocol* p;

    pc = (AbstractProtocol* (*)(StreamBase*, AbstractProtocol*))
        factory.value(protoNumber);
    
    Q_ASSERT_X(pc != NULL, 
               __FUNCTION__, 
               QString("No Protocol Creator registered for protocol %1")
                    .arg(protoNumber).toAscii().constData());

    p = (*pc)(stream, parent);

    return p;
}

AbstractProtocol* ProtocolManager::createProtocol(QString protoName,
    StreamBase *stream, AbstractProtocol *parent)
{
    return createProtocol(nameToNumberMap.value(protoName), stream, parent);
}

bool ProtocolManager::isValidNeighbour(int protoPrefix, int protoSuffix)
{
    if (neighbourProtocols.contains(protoPrefix, protoSuffix))
        return true;
    else
        return false;
}

bool ProtocolManager::protocolHasPayload(int protoNumber)
{
    Q_ASSERT(protocolList.contains(protoNumber));

    return protocolList.value(protoNumber)->protocolHasPayload();
}

QStringList ProtocolManager::protocolDatabase()
{
    return numberToNameMap.values();
}
