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

#include <qendian.h>
#include <QHostAddress>

#include "udp.h"

UdpConfigForm::UdpConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

UdpProtocol::UdpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

UdpProtocol::~UdpProtocol()
{
    delete configForm;
}

AbstractProtocol* UdpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new UdpProtocol(stream, parent);
}

quint32 UdpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kUdpFieldNumber;
}

void UdpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::udp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void UdpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::udp))
        data.MergeFrom(protocol.GetExtension(OstProto::udp));
}

QString UdpProtocol::name() const
{
    return QString("User Datagram Protocol");
}

QString UdpProtocol::shortName() const
{
    return QString("UDP");
}

quint32 UdpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x11;
        default: break;
    }

    return AbstractProtocol::protocolId(type);
}

int    UdpProtocol::fieldCount() const
{
    return udp_fieldCount;
}

AbstractProtocol::FieldFlags UdpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case udp_srcPort:
        case udp_dstPort:
        case udp_totLen:
            break;

        case udp_cksum:
            flags |= FieldIsCksum;
            break;

        case udp_isOverrideTotLen:
        case udp_isOverrideCksum:
            flags |= FieldIsMeta;
            break;

        default:
            break;
    }

    return flags;
}

QVariant UdpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case udp_srcPort:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Source Port");
                case FieldValue:
                    return data.src_port();
                case FieldTextValue:
                    return QString("%1").arg(data.src_port());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.src_port(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        case udp_dstPort:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination Port");
                case FieldValue:
                    return data.dst_port();
                case FieldTextValue:
                    return QString("%1").arg(data.dst_port());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.dst_port(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;

        case udp_totLen:
        {

            switch(attrib)
            {
                case FieldName:            
                    return QString("Datagram Length");
                case FieldValue:
                {
                    int totlen;

                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    return totlen;
                }
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int totlen;
                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    fv.resize(2);
                    qToBigEndian((quint16) totlen, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                {
                    int totlen;
                    totlen = data.is_override_totlen() ? 
                        data.totlen() : 
                        (protocolFramePayloadSize(streamIndex) + 8);
                    return QString("%1").arg(totlen);
                }
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case udp_cksum:
        {
            quint16 cksum;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                {
                    if (data.is_override_cksum())
                        cksum = data.cksum();
                    else
                        cksum = protocolFrameCksum(streamIndex, CksumTcpUdp);
                    qDebug("UDP cksum = %hu", cksum);
                    break;
                }
                default:
                    cksum = 0;
                    break;
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Checksum");
                case FieldValue:
                    return cksum;
                case FieldFrameValue:
                {
                    QByteArray fv;

                    fv.resize(2);
                    qToBigEndian(cksum, (uchar*) fv.data());
                    return fv;
                }
                case FieldTextValue:
                    return  QString("0x%1").
                        arg(cksum, 4, BASE_HEX, QChar('0'));;
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        // Meta fields
        case udp_isOverrideTotLen:
        case udp_isOverrideCksum:
            switch(attrib)
            {
                case FieldIsMeta:
                    return true;
                default:
                    break;
            }
            break;

        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool UdpProtocol::setFieldData(int /*index*/, const QVariant &/*value*/, 
        FieldAttrib /*attrib*/)
{
    //! implement UdpProtocol::setFieldData() 
    return false;
}

bool UdpProtocol::isProtocolFrameValueVariable() const
{
    if (data.is_override_totlen() && data.is_override_cksum())
        return false;
    else
        return isProtocolFramePayloadValueVariable();
}

QWidget* UdpProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new UdpConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void UdpProtocol::loadConfigWidget()
{
    configWidget();

    configForm->leUdpSrcPort->setText(fieldData(udp_srcPort, FieldValue).toString());
    configForm->leUdpDstPort->setText(fieldData(udp_dstPort, FieldValue).toString());

    configForm->leUdpLength->setText(fieldData(udp_totLen, FieldValue).toString());
    configForm->cbUdpLengthOverride->setChecked(data.is_override_totlen());

    configForm->leUdpCksum->setText(QString("%1").arg(
        fieldData(udp_cksum, FieldValue).toUInt(), 4, BASE_HEX, QChar('0')));
    configForm->cbUdpCksumOverride->setChecked(data.is_override_cksum());
}

void UdpProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    data.set_src_port(configForm->leUdpSrcPort->text().toULong(&isOk));
    data.set_dst_port(configForm->leUdpDstPort->text().toULong(&isOk));

    data.set_totlen(configForm->leUdpLength->text().toULong(&isOk));
    data.set_is_override_totlen(configForm->cbUdpLengthOverride->isChecked());

    data.set_cksum(configForm->leUdpCksum->text().remove(QChar(' ')).toULong(&isOk, BASE_HEX));
    data.set_is_override_cksum(configForm->cbUdpCksumOverride->isChecked());
}

