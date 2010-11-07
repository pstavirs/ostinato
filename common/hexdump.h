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

#ifndef _HEXDUMP_H
#define _HEXDUMP_H

#include "hexdump.pb.h"
#include "ui_hexdump.h"

#include "abstractprotocol.h"

/* 
HexDump Protocol Frame Format -
    +---------+---------+
    |   User  |   Zero  |
    | HexDump | Padding |
    +---------+---------+
*/

class HexDumpConfigForm : public QWidget, public Ui::HexDump
{
    Q_OBJECT
public:
    HexDumpConfigForm(QWidget *parent = 0);
private slots:
    void on_hexEdit_overwriteModeChanged(bool isOverwriteMode);
};

class HexDumpProtocol : public AbstractProtocol
{
public:
    HexDumpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~HexDumpProtocol();

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
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

private:
    OstProto::HexDump    data;
    HexDumpConfigForm    *configForm;
    enum hexDumpfield
    {
        // Frame Fields
        hexDump_content = 0,

        // Meta Fields
        hexDump_pad_until_end,

        hexDump_fieldCount
    };
};
#endif
