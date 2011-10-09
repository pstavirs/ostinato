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

#include <QHash>

/* 
Gmp Protocol Frame Format - TODO: for now see the respective RFCs
*/
class GmpProtocol;

class GmpConfigForm : public QWidget, public Ui::Gmp
{
    Q_OBJECT
public:
    GmpConfigForm(QWidget *parent = 0);
    ~GmpConfigForm();
    void update();
protected:
    QString _defaultGroupIp;
    QString _defaultSourceIp;
    enum {
        kSsmQueryPage = 0,
        kSsmReportPage = 1
    };
private slots:
    void on_groupMode_currentIndexChanged(int index);
    void on_addSource_clicked();
    void on_deleteSource_clicked();

    void on_addGroupRecord_clicked();
    void on_deleteGroupRecord_clicked();
    void on_groupList_currentItemChanged(QListWidgetItem *current,
            QListWidgetItem *previous);
    void on_addGroupRecordSource_clicked();
    void on_deleteGroupRecordSource_clicked();
    void on_auxData_textChanged(const QString &text);
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
    virtual int protocolFrameVariableCount() const;

    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

protected:
    enum GmpField
    {
      // ------------
      // Frame Fields
      // ------------
        // Fields used in all ASM and SSM messages, unless otherwise specified
        kType = 0,
        kRsvdMrtCode,
        kChecksum,
        kMldMrt,    // MLD Only (except MLDv2 Report)
        kMldRsvd,   // MLD Only (except MLDv2 Report)
       
        // Field used in ASM messages
        kGroupAddress,
        FIELD_COUNT_ASM_ALL,

        // Fields used in SSM Query
        kRsvd1 = FIELD_COUNT_ASM_ALL,
        kSFlag,
        kQrv,
        kQqic,
        kSourceCount,
        kSources,
        FIELD_COUNT_SSM_QUERY,

        // Fields used in SSM Report
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

    int msgType() const;

    virtual bool isSsmReport() const = 0;
    virtual bool isQuery() const = 0;
    virtual bool isSsmQuery() const = 0;

    int qqic(int value) const;

    virtual quint16 checksum(int streamIndex) const = 0;
private:
    static QHash<int, int> frameFieldCountMap;
};

inline int GmpProtocol::msgType() const 
{ 
    return fieldData(kType, FieldValue).toInt(); 
}

inline int GmpProtocol::qqic(int value) const
{
    return quint8(value); // TODO: if value > 128 convert to mantissa/exp form
}

#endif
