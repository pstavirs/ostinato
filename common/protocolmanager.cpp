#include "protocolmanager.h"
#include "abstractprotocol.h"

#include "protocol.pb.h"
#include "mac.h"
#include "payload.h"
#include "eth2.h"    
#include "dot3.h"    
#include "llc.h"    
#include "snap.h"    
#include "dot2llc.h"
#include "dot2snap.h"
#include "vlan.h"    
#include "vlanstack.h"    
#include "ip4.h"    
#include "tcp.h"    
#include "udp.h"    
#include "userscript.h"
#include "sample.h"

ProtocolManager OstProtocolManager;

ProtocolManager::ProtocolManager()
{
    /*! \todo (LOW) calls to registerProtocol() should be done by the protocols
     themselves (once this is done remove the #includes for all the protocols)
     */
    registerProtocol(OstProto::Protocol::kMacFieldNumber,
            (void*) MacProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kPayloadFieldNumber,
               (void*) PayloadProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kEth2FieldNumber,
               (void*) Eth2Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot3FieldNumber,
               (void*) Dot3Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kLlcFieldNumber,
               (void*) LlcProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSnapFieldNumber,
               (void*) SnapProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot2LlcFieldNumber,
            (void*) Dot2LlcProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kDot2SnapFieldNumber,
            (void*) Dot2SnapProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSvlanFieldNumber,
               (void*) SVlanProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kVlanFieldNumber,
               (void*) VlanProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kVlanStackFieldNumber,
            (void*) VlanStackProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kIp4FieldNumber,
               (void*) Ip4Protocol::createInstance);
    registerProtocol(OstProto::Protocol::kTcpFieldNumber,
               (void*) TcpProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kUdpFieldNumber,
               (void*) UdpProtocol::createInstance);

    registerProtocol(OstProto::Protocol::kUserScriptFieldNumber,
               (void*) UserScriptProtocol::createInstance);
    registerProtocol(OstProto::Protocol::kSampleFieldNumber,
               (void*) SampleProtocol::createInstance);

    populateNeighbourProtocols();
}

ProtocolManager::~ProtocolManager()
{
    numberToNameMap.clear();
    nameToNumberMap.clear();
    neighbourProtocols.clear();
    factory.clear();
    while (!protocolList.isEmpty())
        delete protocolList.takeFirst();
}

void ProtocolManager::registerProtocol(int protoNumber,
    void *protoInstanceCreator)
{
    AbstractProtocol *p;

    Q_ASSERT(!factory.contains(protoNumber));

    factory.insert(protoNumber, protoInstanceCreator);

    p = createProtocol(protoNumber, NULL);
    protocolList.append(p);

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

AbstractProtocol* ProtocolManager::createProtocol(int protoNumber,
    StreamBase *stream, AbstractProtocol *parent)
{
    AbstractProtocol* (*pc)(StreamBase*, AbstractProtocol*);
    AbstractProtocol* p;

    pc = (AbstractProtocol* (*)(StreamBase*, AbstractProtocol*))
        factory.value(protoNumber);
    
    Q_ASSERT(pc != NULL);

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

QStringList ProtocolManager::protocolDatabase()
{
    return numberToNameMap.values();
}
