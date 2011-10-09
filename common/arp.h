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

#ifndef _ARP_H
#define _ARP_H

#include "arp.pb.h"
#include "ui_arp.h"

#include "abstractprotocol.h"

/* 
Arp Protocol Frame Format -
    +------+------+------+------+------+---------+-------+---------+-------+
    | HTYP | PTYP | HLEN | PLEN | OPER |   SHA   |  SPA  |   THA   |  TPA  |
    | (2)  | (2)  | (1)  | (1)  | (2)  |   (6)   |  (4)  |   (6)   |  (4)  |
    +------+------+------+------+------+---------+-------+---------+-------+
Figures in brackets represent field width in bytes
*/

class ArpConfigForm : public QWidget, public Ui::Arp
{
    Q_OBJECT
public:
    ArpConfigForm(QWidget *parent = 0);
private slots:
    void on_senderHwAddrMode_currentIndexChanged(int index);
    void on_senderProtoAddrMode_currentIndexChanged(int index);
    void on_targetHwAddrMode_currentIndexChanged(int index);
    void on_targetProtoAddrMode_currentIndexChanged(int index);
};

class ArpProtocol : public AbstractProtocol
{
private:
    OstProto::Arp    data;
    ArpConfigForm    *configForm;
    enum arpfield
    {
        // Frame Fields
        arp_hwType,
        arp_protoType,

        arp_hwAddrLen,
        arp_protoAddrLen,

        arp_opCode,

        arp_senderHwAddr,
        arp_senderProtoAddr,
        arp_targetHwAddr,
        arp_targetProtoAddr,

        // Meta Fields
        arp_senderHwAddrMode,
        arp_senderHwAddrCount,

        arp_senderProtoAddrMode,
        arp_senderProtoAddrCount,
        arp_senderProtoAddrMask,

        arp_targetHwAddrMode,
        arp_targetHwAddrCount,

        arp_targetProtoAddrMode,
        arp_targetProtoAddrCount,
        arp_targetProtoAddrMask,


        arp_fieldCount
    };

public:
    ArpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~ArpProtocol();

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

    virtual bool isProtocolFrameValueVariable() const;
    virtual int protocolFrameVariableCount() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
