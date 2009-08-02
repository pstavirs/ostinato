#include "protocolmanager.h"

// FIXME(HI): remove
#include "protocol.pb.h"
#include "abstractprotocol.h"

#include "mac.h"
#include "payload.h"

#include "eth2.h"	
#include "dot3.h"	
#include "llc.h"	
#include "snap.h"	
#include "vlan.h"	
#include "ip4.h"	
#include "tcp.h"	
#include "udp.h"	

ProtocolManager OstProtocolManager;

ProtocolManager::ProtocolManager()
{
	registerProtocol(51, QString("mac"),  (void*) MacProtocol::createInstance);
	registerProtocol(52, QString("payload"), (void*) PayloadProtocol::createInstance);
	registerProtocol(121, QString("eth2"), (void*) Eth2Protocol::createInstance);
	registerProtocol(122, QString("dot3"), (void*) Dot3Protocol::createInstance);
	registerProtocol(123, QString("llc"),  (void*) LlcProtocol::createInstance);
	registerProtocol(124, QString("snap"), (void*) SnapProtocol::createInstance);
	registerProtocol(126, QString("vlan"), (void*) VlanProtocol::createInstance);
	registerProtocol(130, QString("ip4"),  (void*) Ip4Protocol::createInstance);
	registerProtocol(140, QString("tcp"),  (void*) TcpProtocol::createInstance);
	registerProtocol(141, QString("udp"),  (void*) UdpProtocol::createInstance);
}

void ProtocolManager::registerProtocol(int protoNumber, QString protoName,
	void *protoInstanceCreator)
{
	// TODO: validate incoming params for duplicates with existing
	nameToNumberMap.insert(protoName, protoNumber);
	numberToNameMap.insert(protoNumber, protoName);
	factory.insert(protoNumber, protoInstanceCreator);
}

AbstractProtocol* ProtocolManager::createProtocol(int protoNumber,
	StreamBase *stream)
{
	AbstractProtocol* (*pc)(StreamBase*);
	AbstractProtocol* p;

	pc = (AbstractProtocol* (*)(StreamBase*))
		factory.value(protoNumber);
	
	Q_ASSERT(pc != NULL);

	p = (*pc)(stream);

	return p;
}

AbstractProtocol* ProtocolManager::createProtocol(QString protoName,
	StreamBase *stream)
{
	return createProtocol(nameToNumberMap.value(protoName), stream);
}

QStringList ProtocolManager::protocolDatabase()
{
	return numberToNameMap.values();
}
