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
    QMap<int, AbstractProtocol*>    protocolList;

    void populateNeighbourProtocols();

public:
    ProtocolManager();
    ~ProtocolManager();

    // TODO: make registerProtocol static
    void registerProtocol(int protoNumber, void *protoInstanceCreator);

    bool isRegisteredProtocol(int protoNumber);
    AbstractProtocol* createProtocol(int protoNumber, StreamBase *stream,
        AbstractProtocol *parent = 0);
    AbstractProtocol* createProtocol(QString protoName, StreamBase *stream,
        AbstractProtocol *parent = 0);

    bool isValidNeighbour(int protoPrefix, int protoSuffix);
    bool protocolHasPayload(int protoNumber);

    QStringList protocolDatabase();
};

#endif
