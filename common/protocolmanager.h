#ifndef _PROTOCOL_MANAGER_H
#define _PROTOCOL_MANAGER_H

#include <QMap>

class ProtocolManager
{
public:
	static QMap<QString, int>	nameToNumberMap;
	static QMap<int, void*>		factory;

public:
	ProtocolManager();
	void registerProtocol(int protoNumber, QString protoName,
		void *protoCreator);
};

#endif
