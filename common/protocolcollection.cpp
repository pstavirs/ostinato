#include "protocolcollection.h"

extern ProtocolManager OstProtocolManager;

ProtocolCollection::ProtocolCollection(ProtocolList &streamProtocols,
	OstProto::StreamCore *streamCore)
	: protoManager(OstProtocolManager)
{
	// Create an instance of each registered protocol

	QMapIterator<int, void*> iter(protoManager.factory);

	while (iter.hasNext())
	{
		AbstractProtocol* (*p)(ProtocolList&, OstProto::StreamCore*);
		AbstractProtocol* q;

		iter.next();
		p = (AbstractProtocol* (*)(ProtocolList&, OstProto::StreamCore*))
			iter.value();
		q = (*p)(streamProtocols, streamCore);

		protocols.insert(iter.key(), q);
	}
}

ProtocolCollection::~ProtocolCollection()
{
	QMutableMapIterator<int,AbstractProtocol*> iter(protocols);

	while (iter.hasNext())
	{
		iter.next();
		if (iter.value())
		{
			delete iter.value();
			iter.remove();
		}
	}
}

void ProtocolCollection::protoDataCopyFrom(const OstProto::Stream &stream) const
{
	QMapIterator<int,AbstractProtocol*> iter(protocols);

	while (iter.hasNext())
	{
		iter.next();
		if (iter.value())
		{
			iter.value()->protoDataCopyFrom(stream);
		}
	}
}

void ProtocolCollection::protoDataCopyInto(OstProto::Stream &stream) const
{
	QMapIterator<int,AbstractProtocol*> iter(protocols);

	while (iter.hasNext())
	{
		iter.next();
		if (iter.value())
		{
			iter.value()->protoDataCopyInto(stream);
		}
	}
}

void ProtocolCollection::loadConfigWidgets() const
{
	QMapIterator<int,AbstractProtocol*> iter(protocols);

	while (iter.hasNext())
	{
		iter.next();
		if (iter.value())
		{
			iter.value()->loadConfigWidget();
		}
	}
}

void ProtocolCollection::storeConfigWidgets() const
{
	QMapIterator<int,AbstractProtocol*> iter(protocols);

	while (iter.hasNext())
	{
		iter.next();
		if (iter.value())
		{
			iter.value()->storeConfigWidget();
		}
	}
}

AbstractProtocol* ProtocolCollection::protocol(int protoNum)
{
	return protocols.value(protoNum);
}

AbstractProtocol* ProtocolCollection::protocol(QString protoName)
{
	return protocols.value(protoManager.nameToNumberMap.value(protoName));
}
