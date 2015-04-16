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
#ifndef _MLD_H
#define _MLD_H

#include "gmp.h"
#include "mld.pb.h"

// MLD uses the same msg type value for 'Query' messages across
// versions despite the fields being different. To distinguish 
// Query messages of different versions, we use an additional 
// upper byte
enum MldMsgType
{
    kMldV1Query = 0x82,
    kMldV1Report = 0x83,
    kMldV1Done = 0x84,

    kMldV2Query = 0xFF82,
    kMldV2Report = 0x8F
};

class MldProtocol : public GmpProtocol
{
public:
    MldProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~MldProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
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

inline bool MldProtocol::isSsmReport() const
{
    return (msgType() == kMldV2Report);
}

inline bool MldProtocol::isQuery() const
{
    return ((msgType() == kMldV1Query)
         || (msgType() == kMldV2Query));
}

inline bool MldProtocol::isSsmQuery() const
{
    return (msgType() == kMldV2Query);
}

inline int MldProtocol::mrc(int value) const
{
    return quint16(value); // TODO: if value > 128, convert to mantissa/exp form
}

#endif
