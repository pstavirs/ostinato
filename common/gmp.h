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

#ifndef _GMP_H
#define _GMP_H

#include "gmp.pb.h"
#include "ui_gmp.h"

#include "abstractprotocol.h"

    enum GmpMsgType
    {
        kIgmpV1Query = 0x11,
        kIgmpV1Report = 0x12,

        kIgmpV2Query = 0xFF11,
        kIgmpV2Report = 0x16,
        kIgmpV2Leave = 0x17,

        kIgmpV3Query = 0xFE11,
        kIgmpV3Report = 0x22,

        kMldV1Query = 0x82,
        kMldV1Report = 0x83,
        kMldV1Done = 0x84,

        kMldV2Query = 0xFF82,
        kMldV2Report = 0x8F
    };

/* 
TODO:FIXME
Gmp Protocol Frame Format -
    +-----+------+------+------+------+------+
    |  A  |   B  |  LEN | CSUM |   X  |   Y  |
    | (3) | (13) | (16) | (16) | (32) | (32) |
    +-----+------+------+------+------+------+
Figures in brackets represent field width in bits
*/

class GmpConfigForm : public QWidget, public Ui::Gmp
{
    Q_OBJECT
public:
    GmpConfigForm(QWidget *parent = 0);
private slots:
    void on_msgTypeCombo_currentIndexChanged(int index);
    void on_addSource_clicked();
    void on_deleteSource_clicked();
};

class GmpProtocol : public AbstractProtocol
{
public:
    GmpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~GmpProtocol();

    virtual ProtocolIdType protocolIdType() const;

    virtual int fieldCount() const;
    virtual int frameFieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual bool isProtocolFrameValueVariable() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

protected:
    enum GmpField
    {
      // ------------
      // Frame Fields
      // ------------
        kType = 0,
        kRsvdMrtCode,
        kChecksum,
        kMldMrt,    // MLD Only (except MLDv2 Report)
        kMldRsvd,   // MLD Only (except MLDv2 Report)
       
        // Used ONLY in - 
        //   IGMPv1: Query, Report
        //   IGMPv2: Report, Leave (v2 uses v1 Query only)
        //   IGMPv3: Query
        //    MLDv1: Query, Report, Done
        //    MLDv2: Query
        kGroupAddress,
        FIELD_COUNT_ASM_ALL,

        // Used ONLY in -
        //   IGMPv3: Query
        //    MLDv2: Query
        kRsvd1 = FIELD_COUNT_ASM_ALL,
        kSFlag,
        kQrv,
        kQqic,
        kSourceCount,
        kSources,
        FIELD_COUNT_SSM_QUERY,

        // Used ONLY in -
        //   IGMPv3: Report
        //    MLDv2: Report
        kRsvd2 = FIELD_COUNT_SSM_QUERY,
        kGroupRecordCount,
        kGroupRecords,
        FIELD_COUNT_SSM_REPORT,
        FRAME_FIELD_COUNT = FIELD_COUNT_SSM_REPORT,

      // -----------
      // Meta Fields
      // -----------
        kIsOverrideChecksum = FRAME_FIELD_COUNT,

        kGroupMode,
        kGroupCount,
        kGroupPrefix,

        kIsOverrideSourceCount,

        kIsOverrideGroupRecordCount,

        FIELD_COUNT
    };

    OstProto::Gmp    data;
    GmpConfigForm    *configForm;

    GmpMsgType msgType() const 
    { 
        return GmpMsgType(fieldData(kType, FieldValue).toUInt()); 
    }
    bool isSsmReport() const
    {
        return ((msgType() == kIgmpV3Report) 
             || (msgType() == kMldV2Report ));
    }
    bool isQuery() const
    {
        return ((msgType() == kIgmpV1Query) 
             || (msgType() == kIgmpV2Query)
             || (msgType() == kIgmpV3Query)
             || (msgType() == kMldV1Query )
             || (msgType() == kMldV2Query ));
    }
    bool isSsmQuery() const
    {
        return ((msgType() == kIgmpV3Query) 
             || (msgType() == kMldV2Query ));
    }

    virtual quint16 checksum(int streamIndex) const = 0;
};

#endif
