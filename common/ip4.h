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

#ifndef _IPV4_H
#define _IPV4_H

#include "abstractprotocol.h"

#include "ip4.pb.h"
#include "ui_ip4.h"

#define IP_FLAG_MF        0x1
#define IP_FLAG_DF        0x2
#define IP_FLAG_UNUSED    0x4


class Ip4ConfigForm : public QWidget, public Ui::ip4
{
    Q_OBJECT
public:
    Ip4ConfigForm(QWidget *parent = 0);
    ~Ip4ConfigForm();
private slots:
    void on_cmbIpSrcAddrMode_currentIndexChanged(int index);
    void on_cmbIpDstAddrMode_currentIndexChanged(int index);
};

class Ip4Protocol : public AbstractProtocol
{
private:
    OstProto::Ip4    data;
    Ip4ConfigForm    *configForm;
    enum ip4field
    {
        ip4_ver = 0,
        ip4_hdrLen,
        ip4_tos,
        ip4_totLen,
        ip4_id,
        ip4_flags,
        ip4_fragOfs,
        ip4_ttl,
        ip4_proto,
        ip4_cksum,
        ip4_srcAddr,
        ip4_dstAddr,

        ip4_isOverrideVer,
        ip4_isOverrideHdrLen,
        ip4_isOverrideTotLen,
        ip4_isOverrideProto,
        ip4_isOverrideCksum,

        ip4_srcAddrMode,
        ip4_srcAddrCount,
        ip4_srcAddrMask,

        ip4_dstAddrMode,
        ip4_dstAddrCount,
        ip4_dstAddrMask,

        ip4_fieldCount
    };

public:
    Ip4Protocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~Ip4Protocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;
    virtual ProtocolIdType protocolIdType() const;
    virtual quint32 protocolId(ProtocolIdType type) const;
    virtual int    fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual bool isProtocolFrameValueVariable() const;
    virtual int protocolFrameVariableCount() const;

    virtual quint32 protocolFrameCksum(int streamIndex = 0,
        CksumType cksumType = CksumIp) const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};


#endif
