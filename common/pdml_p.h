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
#include <QString>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>

// TODO: add const where possible

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
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void prematureEndHandler(int pos, OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto, 
            OstProto::Stream *stream);

    void fieldHandler(QString name, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    void knownFieldHandler(QString name, QString valueHexStr,
            OstProto::Protocol *pbProto);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);

protected:
    QString pdmlProtoName_; // TODO: needed? duplicated in protocolMap_
    int ostProtoId_;
    QMap<QString, int> fieldMap_;
};

class PdmlUnknownProtocol;
class PcapFileFormat;
class PdmlReader : public QObject, public QXmlStreamReader
{
    Q_OBJECT
    //friend class PdmlUnknownProtocol;
public:
    PdmlReader(OstProto::StreamConfigList *streams);
    ~PdmlReader();

    bool read(QIODevice *device, PcapFileFormat *pcap = NULL, 
            bool *stop = NULL);
signals:
    void progress(int value);

private:
    PdmlDefaultProtocol* allocPdmlProtocol(QString protoName);
    void freePdmlProtocol(PdmlDefaultProtocol *proto);

    bool isDontCareProto();
    void skipElement();

    void readPdml();
    void readPacket();
    void readProto();
    void readField(PdmlDefaultProtocol *pdmlProto, 
            OstProto::Protocol *pbProto);

    void appendHexDumpProto(int offset, int size);
    PdmlDefaultProtocol* appendPdmlProto(const QString &protoName,
            OstProto::Protocol **pbProto);

    typedef PdmlDefaultProtocol* (*FactoryMethod)();

    QMap<QString, FactoryMethod> factory_;

    bool *stop_;
    OstProto::StreamConfigList *streams_;
    PcapFileFormat *pcap_;
    QByteArray pktBuf_;

    int packetCount_;
    int expPos_;
    bool skipUntilEnd_;
    OstProto::Stream *prevStream_;
    OstProto::Stream *currentStream_;
};

class PdmlUnknownProtocol : public PdmlDefaultProtocol
{
public:
    PdmlUnknownProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void prematureEndHandler(int pos, OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
private:
    int endPos_;
    int expPos_;
};

class PdmlGenInfoProtocol : public PdmlDefaultProtocol
{
public:
    PdmlGenInfoProtocol();

    static PdmlDefaultProtocol* createInstance();
};

class PdmlFrameProtocol : public PdmlDefaultProtocol
{
public:
    PdmlFrameProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
};

class PdmlEthProtocol : public PdmlDefaultProtocol
{
public:
    PdmlEthProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
};

class PdmlSvlanProtocol : public PdmlDefaultProtocol
{
public:
    PdmlSvlanProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
};

class PdmlVlanProtocol : public PdmlDefaultProtocol
{
public:
    PdmlVlanProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
};

class PdmlLlcProtocol : public PdmlDefaultProtocol
{
public:
    PdmlLlcProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
};

class PdmlArpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlArpProtocol();

    static PdmlDefaultProtocol* createInstance();
};

class PdmlIp4Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp4Protocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
private:
    QByteArray options_;
};

class PdmlIp6Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp6Protocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
};

class PdmlIcmpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlIcmpProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
private:
    static const uint kIcmpInvalidType   = 0xFFFFFFFF;

    static const uint kIcmp6EchoRequest = 128;
    static const uint kIcmp6EchoReply   = 129;
};

class PdmlTcpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlTcpProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
private:
    QByteArray options_;
    QByteArray segmentData_;
};

class PdmlUdpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlUdpProtocol();

    static PdmlDefaultProtocol* createInstance();
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
};

class PdmlTextProtocol : public PdmlDefaultProtocol
{
public:
    PdmlTextProtocol();

    static PdmlDefaultProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
private:
    enum ContentType {
        kUnknownContent,
        kTextContent,
        kOtherContent
    };

    bool detectEol_;
    ContentType contentType_;
    int expPos_;
    int endPos_;
};

#endif
