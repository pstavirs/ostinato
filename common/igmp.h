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

#include "igmp.pb.h"
#include "gmp.h"

class IgmpConfigForm : public GmpConfigForm 
{
public:
    IgmpConfigForm(QWidget *parent = 0);
private slots:
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

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

protected:
    virtual quint16 checksum(int streamIndex) const;
private:
    int mrc(int value) const;
};

inline int IgmpProtocol::mrc(int value) const
{
    return quint8(value); // TODO: if value > 128, convert to mantissa/exp form
}

#endif
