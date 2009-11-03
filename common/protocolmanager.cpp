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

ProtocolManager OstProtocolManager;

ProtocolManager::ProtocolManager()
{
	/*! \todo (LOW) calls to registerProtocol() should be done by the protocols
	 themselves (once this is done remove the #includes for all the protocols)
	 */
	registerProtocol(OstProto::Protocol::kMacFieldNumber,
			QString("mac"),  (void*) MacProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kPayloadFieldNumber,
		   	QString("payload"), (void*) PayloadProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kEth2FieldNumber,
		   	QString("eth2"), (void*) Eth2Protocol::createInstance);
	registerProtocol(OstProto::Protocol::kDot3FieldNumber,
		   	QString("dot3"), (void*) Dot3Protocol::createInstance);
	registerProtocol(OstProto::Protocol::kLlcFieldNumber,
		   	QString("llc"),  (void*) LlcProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kSnapFieldNumber,
		   	QString("snap"), (void*) SnapProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kDot2LlcFieldNumber,
			QString("dot2Llc"), (void*) Dot2LlcProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kDot2SnapFieldNumber,
			QString("dot2Snap"), (void*) Dot2SnapProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kSvlanFieldNumber,
		   	QString("svlan"), (void*) VlanProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kVlanFieldNumber,
		   	QString("vlan"), (void*) VlanProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kVlanStackFieldNumber,
			QString("vlanstack"), (void*) VlanStackProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kIp4FieldNumber,
		   	QString("ip4"),  (void*) Ip4Protocol::createInstance);
	registerProtocol(OstProto::Protocol::kTcpFieldNumber,
		   	QString("tcp"),  (void*) TcpProtocol::createInstance);
	registerProtocol(OstProto::Protocol::kUdpFieldNumber,
		   	QString("udp"),  (void*) UdpProtocol::createInstance);
}

void ProtocolManager::registerProtocol(int protoNumber, QString protoName,
	void *protoInstanceCreator)
{
	//! \todo (MED) validate incoming params for duplicates with existing
	nameToNumberMap.insert(protoName, protoNumber);
	numberToNameMap.insert(protoNumber, protoName);
	factory.insert(protoNumber, protoInstanceCreator);
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

QStringList ProtocolManager::protocolDatabase()
{
	return numberToNameMap.values();
}
