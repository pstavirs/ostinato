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

#include "icmp.pb.h"
#include "ui_icmp.h"

#include "abstractprotocol.h"

/* 
Icmp Protocol Frame Format -
    +-----+------+------+-----+-----+
    | TYP | CODE | CSUM | ID  | SEQ |
    | (1) | (1)  | (2)  | (2) | (2) |
    +-----+------+------+-----+-----+
Figures in brackets represent field width in bytes
*/

class IcmpConfigForm : public QWidget, public Ui::Icmp
{
    Q_OBJECT
public:
    IcmpConfigForm(QWidget *parent = 0);
private slots:
};

class IcmpProtocol : public AbstractProtocol
{
private:
    OstProto::Icmp    data;
    IcmpConfigForm    *configForm;
    enum icmpfield
    {
        // Frame Fields
        icmp_type = 0,
        icmp_code,
        icmp_checksum,
        icmp_identifier,
        icmp_sequence,

        // Meta Fields
        icmp_is_override_checksum,

        icmp_fieldCount
    };

public:
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

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
