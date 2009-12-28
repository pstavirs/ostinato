#ifndef _UDP_H
#define _UDP_H

#include "abstractprotocol.h"

#include "udp.pb.h"
#include "ui_udp.h"

class UdpConfigForm : public QWidget, public Ui::udp
{
    Q_OBJECT
public:
    UdpConfigForm(QWidget *parent = 0);
};

class UdpProtocol : public AbstractProtocol
{
private:
    OstProto::Udp    data;
    UdpConfigForm    *configForm;
    enum udpfield
    {
        udp_srcPort = 0,
        udp_dstPort,
        udp_totLen,
        udp_cksum,

        udp_isOverrideTotLen,
        udp_isOverrideCksum,

        udp_fieldCount
    };

public:
    UdpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~UdpProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;
    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual int    fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual bool isProtocolFrameValueVariable() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
