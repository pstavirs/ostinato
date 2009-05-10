#include "protocolmanager.h"

#include "mac.h"
#include "payload.h"

#include "eth2.h"	
#include "dot3.h"	
#include "llc.h"	
#include "snap.h"	
#include "ip4.h"	
#include "tcp.h"	
#include "udp.h"	

QMap<int, void*>	ProtocolManager::factory;
QMap<QString, int>	ProtocolManager::nameToNumberMap;

ProtocolManager OstProtocolManager;

ProtocolManager::ProtocolManager()
{
	registerProtocol(51, QString("mac"),  (void*) MacProtocol::createInstance);
	registerProtocol(52, QString("payload"), (void*) PayloadProtocol::createInstance);
	registerProtocol(121, QString("eth2"), (void*) Eth2Protocol::createInstance);
	registerProtocol(122, QString("dot3"), (void*) Dot3Protocol::createInstance);
	registerProtocol(123, QString("llc"),  (void*) LlcProtocol::createInstance);
	registerProtocol(124, QString("snap"), (void*) SnapProtocol::createInstance);
	registerProtocol(130, QString("ip4"),  (void*) Ip4Protocol::createInstance);
	registerProtocol(140, QString("tcp"),  (void*) TcpProtocol::createInstance);
	registerProtocol(141, QString("udp"),  (void*) UdpProtocol::createInstance);
}

void ProtocolManager::registerProtocol(int protoNumber, QString protoName,
	void *protoInstanceCreator)
{
	// TODO: validate incoming params for duplicates with existing
	nameToNumberMap.insert(protoName, protoNumber);
	factory.insert(protoNumber, protoInstanceCreator);
}
