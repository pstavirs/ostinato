/*
Copyright (C) 2011 Srivats P.

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

#ifndef _PDML_READER_H
#define _PDML_READER_H

#include "pdmlprotocol.h"

#include <QByteArray>
#include <QXmlStreamReader>

class PcapFileFormat;
class PdmlReader : public QObject, public QXmlStreamReader
{
    Q_OBJECT
public:
    PdmlReader(OstProto::StreamConfigList *streams);
    ~PdmlReader();

    bool read(QIODevice *device, PcapFileFormat *pcap = NULL, 
            bool *stop = NULL);
signals:
    void progress(int value);

private:
    PdmlProtocol* allocPdmlProtocol(QString protoName);
    void freePdmlProtocol(PdmlProtocol *proto);

    bool isDontCareProto();
    void skipElement();

    void readPdml();
    void readPacket();
    void readProto();
    void readField(PdmlProtocol *pdmlProto, 
            OstProto::Protocol *pbProto);

    void appendHexDumpProto(int offset, int size);
    PdmlProtocol* appendPdmlProto(const QString &protoName,
            OstProto::Protocol **pbProto);

    typedef PdmlProtocol* (*FactoryMethod)();

    QMap<QString, FactoryMethod> factory_;

    bool *stop_;
    OstProto::StreamConfigList *streams_;
    PcapFileFormat *pcap_;
    QByteArray pktBuf_;

    bool isMldSupport_;
    int packetCount_;
    int expPos_;
    bool skipUntilEnd_;
    OstProto::Stream *prevStream_;
    OstProto::Stream *currentStream_;
};

#endif
