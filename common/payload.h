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

#ifndef _PAYLOAD_H
#define _PAYLOAD_H

#include "abstractprotocol.h"

#include "payload.pb.h"
#include "ui_payload.h"

class PayloadConfigForm : public QWidget, public Ui::payload
{
    Q_OBJECT
public:
    PayloadConfigForm(QWidget *parent = 0);
private slots:
    void on_cmbPatternMode_currentIndexChanged(int index);
};

class PayloadProtocol : public AbstractProtocol
{
private:
    OstProto::Payload            data;
    PayloadConfigForm    *configForm;
    enum payloadfield
    {
        payload_dataPattern,

        // Meta fields
        payload_dataPatternMode,

        payload_fieldCount
    };

public:
    PayloadProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~PayloadProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual int    fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual bool isProtocolFrameValueVariable() const;
    virtual bool isProtocolFrameSizeVariable() const;
    virtual int protocolFrameVariableCount() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
