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

#ifndef _PDML_PROTOCOLS_H
#define _PDML_PROTOCOLS_H

#include "pdmlprotocol.h"

class PdmlUnknownProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

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
protected:
    PdmlUnknownProtocol();

private:
    int endPos_;
    int expPos_;
};

class PdmlGenInfoProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

protected:
    PdmlGenInfoProtocol();

};

class PdmlFrameProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);

protected:
    PdmlFrameProtocol();
};

class PdmlEthProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);

protected:
    PdmlEthProtocol();
};

class PdmlSvlanProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
protected:
    PdmlSvlanProtocol();
};

class PdmlVlanProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
protected:
    PdmlVlanProtocol();
};

class PdmlLlcProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlLlcProtocol();
};

class PdmlArpProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

protected:
    PdmlArpProtocol();
};

class PdmlIp4Protocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlIp4Protocol();
private:
    QByteArray options_;
};

class PdmlIp6Protocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlIp6Protocol();
};

class PdmlIcmpProtocol : public PdmlProtocol
{
    friend class PdmlIcmp6Protocol;
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlIcmpProtocol();
private:
    static const uint kIcmpInvalidType   = 0xFFFFFFFF;

    static const uint kIcmp6EchoRequest = 128;
    static const uint kIcmp6EchoReply   = 129;
};

class PdmlMldProtocol : public PdmlProtocol
{
    friend class PdmlIcmp6Protocol;
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
protected:
    PdmlMldProtocol();
private:
    static const uint kMldQuery = 0x82;
    static const uint kMldV1Query = 0x82;
    static const uint kMldV2Query = 0xFF82;

    uint protoSize_;
};

class PdmlIcmp6Protocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto, 
            OstProto::Stream *stream);

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
protected:
    PdmlIcmp6Protocol();
private:
    PdmlIcmpProtocol icmp_;
    PdmlMldProtocol mld_;
    PdmlProtocol *proto_;
};

class PdmlIgmpProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlIgmpProtocol();
private:
    static const uint kIgmpQuery = 0x11;
    static const uint kIgmpV1Query = 0x11;
    static const uint kIgmpV2Query = 0xFF11;
    static const uint kIgmpV3Query = 0xFE11;

    uint version_;
};

class PdmlTcpProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlTcpProtocol();
private:
    QByteArray options_;
    QByteArray segmentData_;
};

class PdmlUdpProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlUdpProtocol();
};

class PdmlTextProtocol : public PdmlProtocol
{
public:
    static PdmlProtocol* createInstance();

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
protected:
    PdmlTextProtocol();
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
