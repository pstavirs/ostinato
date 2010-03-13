#ifndef _SAMPLE_H
#define _SAMPLE_H

#include "sample.pb.h"
#include "ui_sample.h"

#include "abstractprotocol.h"

/* 
Sample Protocol Frame Format -
    +-----+------+------+------+------+------+
    |  A  |   B  |  LEN | CSUM |   X  |   Y  |
    | (3) | (13) | (16) | (16) | (32) | (32) |
    +-----+------+------+------+------+------+
Figures in brackets represent field width in bits
*/

class SampleConfigForm : public QWidget, public Ui::Sample
{
    Q_OBJECT
public:
    SampleConfigForm(QWidget *parent = 0);
private slots:
};

class SampleProtocol : public AbstractProtocol
{
private:
    OstProto::Sample    data;
    SampleConfigForm    *configForm;
    enum samplefield
    {
        // Frame Fields
        sample_a = 0,
        sample_b,
        sample_payloadLength,
        sample_checksum,
        sample_x,
        sample_y,

        // Meta Fields
        sample_is_override_checksum,

        sample_fieldCount
    };

public:
    SampleProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~SampleProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual ProtocolIdType protocolIdType() const;
    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual bool isProtocolFrameValueVariable() const;
    virtual bool isProtocolFrameSizeVariable() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
