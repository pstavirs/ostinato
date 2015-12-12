/*
Copyright (C) 2010, 2014 Srivats P.
Copyright (C) 2015 Ilya Volchanetskiy

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

#ifndef _MPLS_H
#define _MPLS_H

#include "abstractprotocol.h"
#include "mpls.pb.h"

/* 
MPLS Protocol Frame Format -
    +-------+-----+-----+-----+
    | Label | EXP | BoS | TTL |
    | (20)  | (3) | (1) | (8) |
    +-------+-----+-----+-----+
Figures in brackets represent field width in bits
*/

class MplsProtocol : public AbstractProtocol
{
public:
    enum mplsfield
    {
        // Frame Fields
        mpls_label = 0,
        mpls_exp,
        mpls_bos,
        mpls_ttl,

        // Meta Fields
        mpls_fieldCount
    };

    MplsProtocol(StreamBase *stream, AbstractProtocol *parent = 0);

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

private:
    OstProto::Mpls    data;
};

#endif
