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

#ifndef _VLAN_H
#define _VLAN_H

#include "abstractprotocol.h"
#include "vlan.pb.h"

class VlanProtocol : public AbstractProtocol
{
public:
    enum Vlanfield
    {
        vlan_tpid,
        vlan_prio,
        vlan_cfiDei,
        vlan_vlanId,

        // meta-fields
        vlan_isOverrideTpid,

        vlan_fieldCount
    };

    VlanProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~VlanProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int    fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

protected:
    OstProto::Vlan    data;
};

#endif
