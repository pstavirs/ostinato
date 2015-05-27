/*
Copyright (C) 2010, 2014 Srivats P.

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

#ifndef _SAMPLE_H
#define _SAMPLE_H

#include "abstractprotocol.h"
#include "sample.pb.h"

/* 
Sample Protocol Frame Format -
    +-----+------+------+------+------+------+
    |  A  |   B  |  LEN | CSUM |   X  |   Y  |
    | (3) | (13) | (16) | (16) | (32) | (32) |
    +-----+------+------+------+------+------+
Figures in brackets represent field width in bits
*/

class SampleProtocol : public AbstractProtocol
{
public:
    enum samplefield
    {
        // Frame Fields
        sample_a = 0,
        sample_b,
        sample_payloadLength,
        sample_checksum,
        sample_x,
        sample_y,

        // Meta Fields
        sample_is_override_checksum,

        sample_fieldCount
    };

    SampleProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~SampleProtocol();

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
    virtual int frameFieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual bool isProtocolFrameSizeVariable() const;
    virtual int protocolFrameVariableCount() const;

private:
    OstProto::Sample    data;
};

#endif
