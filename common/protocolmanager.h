#ifndef _PROTOCOL_MANAGER_H
#define _PROTOCOL_MANAGER_H

#include <QMap>
#include <QStringList>

class AbstractProtocol;
class StreamBase;

class ProtocolManager
{
	QMap<int, QString>	numberToNameMap;
	QMap<QString, int>	nameToNumberMap;
	QMap<int, void*>	factory;

public:
	ProtocolManager();

	void registerProtocol(int protoNumber, QString protoName,
		void *protoCreator);

	AbstractProtocol* createProtocol(int protoNumber, StreamBase *stream);
	AbstractProtocol* createProtocol(QString protoName, StreamBase *stream);

	QStringList protocolDatabase();
};

#endif
