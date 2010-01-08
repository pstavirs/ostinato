#ifndef _PROTOCOL_MANAGER_H
#define _PROTOCOL_MANAGER_H

#include <QMap>
#include <QStringList>

class AbstractProtocol;
class StreamBase;

class ProtocolManager
{
    QMap<int, QString>    numberToNameMap;
    QMap<QString, int>    nameToNumberMap;
    QMultiMap<int, int> neighbourProtocols;
    QMap<int, void*>    factory;
    QList<AbstractProtocol*>    protocolList;

    void populateNeighbourProtocols();

public:
    ProtocolManager();
    ~ProtocolManager();

    void registerProtocol(int protoNumber, void *protoInstanceCreator);

    AbstractProtocol* createProtocol(int protoNumber, StreamBase *stream,
        AbstractProtocol *parent = 0);
    AbstractProtocol* createProtocol(QString protoName, StreamBase *stream,
        AbstractProtocol *parent = 0);

    bool isValidNeighbour(int protoPrefix, int protoSuffix);

    QStringList protocolDatabase();
};

#endif
