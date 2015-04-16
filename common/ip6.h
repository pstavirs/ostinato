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

#ifndef _IP6_H
#define _IP6_H

#include "abstractprotocol.h"
#include "ip6.pb.h"

/* 
IPv6 Protocol Frame Format -
    +-----+----------+-----------------------+
    | Ver | TrfClass |       FlowLabel       |
    | (4) |    (8)   |         (20)          |
    +-----+-------------+---------+----------+
    |   Payload Length  | NextHdr | HopLimit |
    |        (16)       |    (8)  |    (8)   |
    +-------------------+---------+----------+
    |                                        |
    |            Source Address              |
    |                (128)                   |
    |                                        |
    +-----+------+------+------+------+------+
    |                                        |
    |         Destination Address            |
    |                (128)                   |
    |                                        |
    +-----+------+------+------+------+------+
Figures in brackets represent field width in bits
*/

class Ip6Protocol : public AbstractProtocol
{
public:
    enum ip6field
    {
        // Frame Fields
        ip6_version = 0,
        ip6_trafficClass,
        ip6_flowLabel,
        ip6_payloadLength,
        ip6_nextHeader,
        ip6_hopLimit,
        ip6_srcAddress,
        ip6_dstAddress,

        // Meta Fields
        ip6_isOverrideVersion,
        ip6_isOverridePayloadLength,
        ip6_isOverrideNextHeader,

        ip6_srcAddrMode,
        ip6_srcAddrCount,
        ip6_srcAddrPrefix,

        ip6_dstAddrMode,
        ip6_dstAddrCount,
        ip6_dstAddrPrefix,

        ip6_fieldCount
    };

    Ip6Protocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~Ip6Protocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual ProtocolIdType protocolIdType() const;
    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual bool isProtocolFrameValueVariable() const;
    virtual int protocolFrameVariableCount() const;

    virtual quint32 protocolFrameCksum(int streamIndex = 0,
            CksumType cksumType = CksumIp) const;
private:
    OstProto::Ip6 data;
};

#endif
