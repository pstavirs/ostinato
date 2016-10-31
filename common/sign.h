/*
Copyright (C) 2016 Srivats P.

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

#ifndef _SIGN_H
#define _SIGN_H

#include "abstractprotocol.h"
#include "sign.pb.h"

/*
Sign Protocol is expected at the end of the frame (just before the Eth FCS)
    --+-------+
 . . .| Magic |
      |  (32) |
    --+-------+
Figures in brackets represent field width in bits
*/

class SignProtocol : public AbstractProtocol
{
public:
    enum samplefield
    {
        // Frame Fields
        sign_magic = 0,

        // Meta Fields
        // - None

        sign_fieldCount
    };

    SignProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~SignProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;

private:
    static const quint32 kSignMagic = 0xa1b2c3d4; // FIXME
    OstProto::Sign data;
};

#endif
