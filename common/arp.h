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

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();
};

#endif
