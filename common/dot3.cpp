#include <qendian.h>
#include <QHostAddress>

#include "dot3.h"
#include "streambase.h"

#define SZ_FCS        4

Dot3ConfigForm::Dot3ConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

Dot3Protocol::Dot3Protocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

Dot3Protocol::~Dot3Protocol()
{
    delete configForm;
}

AbstractProtocol* Dot3Protocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new Dot3Protocol(stream, parent);
}

quint32 Dot3Protocol::protocolNumber() const
{
    return OstProto::Protocol::kDot3FieldNumber;
}

void Dot3Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::dot3)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Dot3Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::dot3))
        data.MergeFrom(protocol.GetExtension(OstProto::dot3));
}

QString Dot3Protocol::name() const
{
    return QString("802.3");
}

QString Dot3Protocol::shortName() const
{
    return QString("802.3");
}

int    Dot3Protocol::fieldCount() const
{
    return dot3_fieldCount;
}

QVariant Dot3Protocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case dot3_length:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Length");
                case FieldValue:
                {
                    quint16 len;

                    //len = mpStream->frameLen() - SZ_FCS;
                    len = protocolFramePayloadSize(streamIndex);
                    return len;
                }
                case FieldTextValue:
                {
                    quint16 len;

                    //len = mpStream->frameLen() - SZ_FCS;
                    len = protocolFramePayloadSize(streamIndex);
                    return QString("%1").arg(len);
                }
                case FieldFrameValue:
                {
                    quint16 len;
                    QByteArray fv;

                    //len = mpStream->frameLen() - SZ_FCS;
                    len = protocolFramePayloadSize(streamIndex);
                    fv.resize(2);
                    qToBigEndian(len, (uchar*) fv.data());
                    return fv;
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;

        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool Dot3Protocol::setFieldData(int /*index*/, const QVariant &/*value*/, 
        FieldAttrib /*attrib*/)
{
    return false;
}

bool Dot3Protocol::isProtocolFrameValueVariable() const
{
    return isProtocolFramePayloadSizeVariable();
}

QWidget* Dot3Protocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new Dot3ConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void Dot3Protocol::loadConfigWidget()
{
    configWidget();

    configForm->leLength->setText(
        fieldData(dot3_length, FieldValue).toString());
}

void Dot3Protocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    data.set_length(configForm->leLength->text().toULong(&isOk));
}

