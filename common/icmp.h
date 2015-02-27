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

#ifndef _ICMP_H
#define _ICMP_H

#include "abstractprotocol.h"
#include "icmp.pb.h"

/* 
Icmp Protocol Frame Format -
    +-----+------+------+------+-------+
    | TYP | CODE | CSUM | [ID] | [SEQ] |
    | (1) | (1)  | (2)  | (2)  |  (2)  |
    +-----+------+------+------+-------+
Fields within [] are applicable only to certain TYPEs
Figures in braces represent field width in bytes
*/

class IcmpProtocol : public AbstractProtocol
{
public:
    enum icmpfield
    {
        // Frame Fields
        icmp_type = 0,
        icmp_code,
        icmp_checksum,
        icmp_commonFrameFieldCount,

        icmp_identifier = icmp_commonFrameFieldCount,
        icmp_sequence,
        icmp_idSeqFrameFieldCount,

        // Meta Fields
        icmp_is_override_checksum = icmp_idSeqFrameFieldCount,
        icmp_version,

        icmp_fieldCount
    };

    IcmpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~IcmpProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;
    virtual int frameFieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

private:
    OstProto::Icmp    data;

    OstProto::Icmp::Version icmpVersion() const
    {
        return OstProto::Icmp::Version(
                fieldData(icmp_version, FieldValue).toUInt());
    }
    int icmpType() const
    {
        return fieldData(icmp_type, FieldValue).toInt();
    }
};

#endif
