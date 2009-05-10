#ifndef _PROTOCOL_COLLECTION_H
#define _PROTOCOL_COLLECTION_H

#include <QMap>
#include <QLinkedList>

#include "abstractprotocol.h"
#include "protocolmanager.h"

class ProtocolCollection {

	ProtocolManager					&protoManager;
	QMap<int, AbstractProtocol*>	protocols;

public:
	ProtocolCollection(ProtocolList &streamProtocols,
		OstProto::StreamCore *streamCore);
	ProtocolCollection::~ProtocolCollection();

	void protoDataCopyFrom(const OstProto::Stream &stream) const;
	void protoDataCopyInto(OstProto::Stream &stream) const;

	void loadConfigWidgets() const;
	void storeConfigWidgets() const;

	AbstractProtocol* protocol(int protoNum);
	AbstractProtocol* protocol(QString protoName);
};

#endif
