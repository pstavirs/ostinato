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

// TODO: add const where possible

class QXmlSimpleReader;
class QXmlInputSource;

class PdmlDefaultProtocol
{
public:
    PdmlDefaultProtocol();
    virtual ~PdmlDefaultProtocol();

    QString pdmlProtoName() const;
    int ostProtoId() const;
    bool hasField(QString name) const;
    int fieldId(QString name) const;

    virtual void preProtocolHandler(QString name, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);

protected:
    QString pdmlProtoName_; // TODO: needed? duplicated in protocolMap_
    int ostProtoId_;
    QMap<QString, int> fieldMap_;
};

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

class PdmlUnknownProtocol : public PdmlDefaultProtocol
{
public:
    PdmlUnknownProtocol();

    virtual void preProtocolHandler(QString name, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
private:
    int endPos_;
    int expPos_;
};

class PdmlGenInfoProtocol : public PdmlDefaultProtocol
{
public:
    PdmlGenInfoProtocol();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
};

class PdmlFrameProtocol : public PdmlDefaultProtocol
{
public:
    PdmlFrameProtocol();
};

#if 1
class PdmlFakeFieldWrapperProtocol : public PdmlDefaultProtocol
{
public:
    PdmlFakeFieldWrapperProtocol();

    virtual void preProtocolHandler(QString name, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
private:
    int expPos_;
};
#endif

class PdmlEthProtocol : public PdmlDefaultProtocol
{
public:
    PdmlEthProtocol();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
};

class PdmlIp4Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp4Protocol();
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
};

class PdmlIp6Protocol : public PdmlDefaultProtocol
{
public:
    PdmlIp6Protocol();
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
};

class PdmlTcpProtocol : public PdmlDefaultProtocol
{
public:
    PdmlTcpProtocol();
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlAttributes &attributes, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Stream *stream);
private:
    QByteArray options_;
    QByteArray segmentData_;
};

#endif
