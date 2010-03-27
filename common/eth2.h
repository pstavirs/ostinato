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

#ifndef _ETH2_H
#define _ETH2_H

#include "abstractprotocol.h"

#include "eth2.pb.h"
#include "ui_eth2.h"

class Eth2ConfigForm : public QWidget, public Ui::eth2
{
    Q_OBJECT
public:
    Eth2ConfigForm(QWidget *parent = 0);
};

class Eth2Protocol : public AbstractProtocol
{
private:
    OstProto::Eth2    data;
    Eth2ConfigForm    *configForm;
    enum eth2field
    {
        eth2_type = 0,

        eth2_fieldCount
    };

public:
    Eth2Protocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~Eth2Protocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;

    virtual ProtocolIdType protocolIdType() const;

    virtual int    fieldCount() const;

    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
