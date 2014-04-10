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
#ifndef _IGMP_H
#define _IGMP_H

#include "gmp.h"
#include "igmp.pb.h"

// IGMP uses the same msg type value for 'Query' messages across
// versions despite the fields being different. To distinguish 
// Query messages of different versions, we use an additional 
// upper byte
enum IgmpMsgType
{
    kIgmpV1Query = 0x11,
    kIgmpV1Report = 0x12,

    kIgmpV2Query = 0xFF11,
    kIgmpV2Report = 0x16,
    kIgmpV2Leave = 0x17,

    kIgmpV3Query = 0xFE11,
    kIgmpV3Report = 0x22,
};

class IgmpProtocol : public GmpProtocol
{
public:
    IgmpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~IgmpProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

protected:
    virtual bool isSsmReport() const;
    virtual bool isQuery() const;
    virtual bool isSsmQuery() const;

    virtual quint16 checksum(int streamIndex) const;

private:
    int mrc(int value) const;
};

inline bool IgmpProtocol::isSsmReport() const
{
    return (msgType() == kIgmpV3Report);
}

inline bool IgmpProtocol::isQuery() const
{
    return ((msgType() == kIgmpV1Query) 
         || (msgType() == kIgmpV2Query)
         || (msgType() == kIgmpV3Query));
}

inline bool IgmpProtocol::isSsmQuery() const
{
    return (msgType() == kIgmpV3Query); 
}

inline int IgmpProtocol::mrc(int value) const
{
    return quint8(value); // TODO: if value > 128, convert to mantissa/exp form
}

#endif
