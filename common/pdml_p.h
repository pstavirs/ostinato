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
#ifndef _PDML_P_H
#define _PDML_P_H

#include "protocol.pb.h"

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>

// TODO: add const where possible

class QXmlSimpleReader;
class QXmlInputSource;

class PdmlDefaultProtocol
{
public:
    PdmlDefaultProtocol(); // TODO: make private
    virtual ~PdmlDefaultProtocol();

    static PdmlDefaultProtocol* createInstance();

    QString pdmlProtoName() const;
    int ostProtoId() const;
    bool hasField(QString name) const;
    int fieldId(QString name) const;

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void prematureEndHandler(int pos, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);

protected:
    QString pdmlProtoName_; // TODO: needed? duplicated in protocolMap_
    int ostProtoId_;
    QMap<QString, int> fieldMap_;
};

#if 0
class PdmlParser : public QXmlDefaultHandler
{
public:
    PdmlParser(OstProto::StreamConfigList *streams);
    ~PdmlParser();

    bool startElement(const QString &namespaceURI,
                      const QString &localName,
                      const QString &qName,
                      const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI,
                    const QString &localName,
                    const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);

private:
    void initProtocolMaps();

    QMap<QString, PdmlDefaultProtocol*> protocolMap_;
    PdmlDefaultProtocol *currentPdmlProtocol_;
    int skipCount_;
    int packetCount_;
    OstProto::StreamConfigList *streams_;

    OstProto::Stream *currentStream_;
    google::protobuf::Message *currentProtocolMsg_;
};
#endif

class PdmlUnknownProtocol;
class PdmlReader : public QXmlStreamReader
{
    friend class PdmlUnknownProtocol;
public:
    PdmlReader(OstProto::StreamConfigList *streams);
    ~PdmlReader();

    bool read(QIODevice *device);

private:
    PdmlDefaultProtocol* allocPdmlProtocol(QString protoName);
    void freePdmlProtocol(PdmlDefaultProtocol *proto);

    bool isDontCareProto();
    void readPdml();
    void skipElement();
    void readUnexpectedElement();

    void readPacketPass1();
    void readProtoPass1();
    void readFieldPass1();

    void readPacket();
    void readProto();
    void readField(PdmlDefaultProtocol *pdmlProto, 
            google::protobuf::Message *pbProto);

    typedef PdmlDefaultProtocol* (*FactoryMethod)();

#if 0
    class PacketFragment // TODO: find a better name!
    {
    public:
    private:
        typedef struct
        {
            int pos;
            int size;
            QByteArray value;
        } Fragment;
        QList<Fragment> 
    };
#endif
    typedef struct
    {
        int pos;
        int size;
        QByteArray value;
    } Fragment;

    QMap<QString, FactoryMethod> factory_;

    OstProto::StreamConfigList *streams_;

    int pass_;
    int packetCount_;
    OstProto::Stream *currentStream_;
    QList<Fragment> pktFragments_; 

    //PdmlDefaultProtocol *currentPdmlProtocol_;
    //google::protobuf::Message *currentProtocolMsg_;
};

class PdmlUnknownProtocol : public PdmlDefaultProtocol
{
public:
    PdmlUnknownProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void prematureEndHandler(int pos, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
private:
    int endPos_;
    int expPos_;
};

class PdmlGenInfoProtocol : public PdmlDefaultProtocol
{
public:
    PdmlGenInfoProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
};

class PdmlFrameProtocol : public PdmlDefaultProtocol
{
public:
    PdmlFrameProtocol();

    static PdmlDefaultProtocol* createInstance();
};

#if 1
class PdmlFakeFieldWrapperProtocol : public PdmlDefaultProtocol
{
public:
    PdmlFakeFieldWrapperProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
private:
    int expPos_;
};
#endif

class PdmlEthProtocol : public PdmlDefaultProtocol
{
public:
    PdmlEthProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
};

class PdmlIp4Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp4Protocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
};

class PdmlIp6Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp6Protocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
};

class PdmlTcpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlTcpProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
private:
    QByteArray options_;
    QByteArray segmentData_;
};

#endif
