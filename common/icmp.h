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
