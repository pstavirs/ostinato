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

#include "icmp.h"

IcmpConfigForm::IcmpConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    typeCombo->setValidator(new QIntValidator(0, 0xFF, this));
    typeCombo->addItem(0, "Echo Reply");
    typeCombo->addItem(3, "Destination Unreachable");
    typeCombo->addItem(4, "Source Quench");
    typeCombo->addItem(5, "Redirect");
    typeCombo->addItem(8, "Echo Request");
    typeCombo->addItem(11, "Time Exceeded");
    typeCombo->addItem(12, "Parameter Problem");
    typeCombo->addItem(13, "Timestamp Request");
    typeCombo->addItem(14, "Timestamp Reply");
    typeCombo->addItem(17, "Address Mask Request");
    typeCombo->addItem(18, "Address Mask Reply");

    idEdit->setValidator(new QIntValidator(0, 0xFFFF, this));
    seqEdit->setValidator(new QIntValidator(0, 0xFFFF, this));
}

IcmpProtocol::IcmpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

IcmpProtocol::~IcmpProtocol()
{
    delete configForm;
}

AbstractProtocol* IcmpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new IcmpProtocol(stream, parent);
}

quint32 IcmpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kIcmpFieldNumber;
}

void IcmpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::icmp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void IcmpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::icmp))
        data.MergeFrom(protocol.GetExtension(OstProto::icmp));
}

QString IcmpProtocol::name() const
{
    return QString("Internet Control Message Protocol");
}

QString IcmpProtocol::shortName() const
{
    return QString("ICMP");
}

quint32 IcmpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x1;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

int IcmpProtocol::fieldCount() const
{
    return icmp_fieldCount;
}

AbstractProtocol::FieldFlags IcmpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case icmp_type:
        case icmp_code:
            break;

        case icmp_checksum:
            flags |= FieldIsCksum;
            break;

        case icmp_identifier:
        case icmp_sequence:
            break;

        case icmp_is_override_checksum:
            flags |= FieldIsMeta;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant IcmpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case icmp_type:
        {
            unsigned char type = data.type() & 0xFF;

            switch(attrib)
            {
                case FieldName:            
                    return QString("Type");
                case FieldValue:
                    return type;
                case FieldTextValue:
                    return QString("%1").arg((uint) type);
                case FieldFrameValue:
                    return QByteArray(1, type);
                default:
                    break;
            }
            break;

        }
        case icmp_code:
        {
            unsigned char code = data.code() & 0xFF;

            switch(attrib)
            {
                case FieldName:            
                    return QString("code");
                case FieldValue:
                    return code;
                case FieldTextValue:
                    return QString("%1").arg((uint)code);
                case FieldFrameValue:
                    return QByteArray(1, code);
                default:
                    break;
            }
            break;

        }
        case icmp_checksum:
        {
            quint16 cksum;

            switch(attrib)
            {
                case FieldValue:
                case FieldFrameValue:
                case FieldTextValue:
                    if (data.is_override_checksum())
                    {
                        cksum = data.checksum();
                    }
                    else
                    {
                        quint16 cks;
                        quint32 sum = 0;

                        cks = protocolFrameCksum(streamIndex, CksumIp);
                        sum += (quint16) ~cks;
                        cks = protocolFramePayloadCksum(streamIndex, CksumIp);
                        sum += (quint16) ~cks;

                        while(sum>>16)
                            sum = (sum & 0xFFFF) + (sum >> 16);

                        cksum = (~sum) & 0xFFFF;
                    }
                    break;
                default:
                    cksum = 0; // avoid the 'maybe used unitialized' warning
                    break;
            }
            printf("%s: attrib = %d, cksum = %d\n", __FUNCTION__, attrib, cksum);

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
                    return  QString("0x%1").arg(
                        cksum, 4, BASE_HEX, QChar('0'));;
                case FieldBitSize:
                    return 16;
                default:
                    break;
            }
            break;
        }
        case icmp_identifier:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Identifier");
                case FieldValue:
                    return data.identifier();
                case FieldTextValue:
                    return QString("%1").arg(data.identifier());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.identifier(), 
                            (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case icmp_sequence:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("Sequence");
                case FieldValue:
                    return data.sequence();
                case FieldTextValue:
                    return QString("%1").arg(data.sequence());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian((quint16) data.sequence(), (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }


        // Meta fields
        case icmp_is_override_checksum:
        {
            switch(attrib)
            {
                case FieldValue:
                    return data.is_override_checksum();
                default:
                    break;
            }
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool IcmpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case icmp_type:
        {
            uint type = value.toUInt(&isOk);
            if (isOk)
                data.set_type(type & 0xFF);
            break;
        }
        case icmp_code:
        {
            uint code = value.toUInt(&isOk);
            if (isOk)
                data.set_code(code & 0xFF);
            break;
        }
        case icmp_checksum:
        {
            uint csum = value.toUInt(&isOk);
            if (isOk)
                data.set_checksum(csum);
            break;
        }
        case icmp_identifier:
        {
            uint id = value.toUInt(&isOk);
            if (isOk)
                data.set_identifier(id);
            break;
        }
        case icmp_sequence:
        {
            uint seq = value.toUInt(&isOk);
            if (isOk)
                data.set_sequence(seq);
            break;
        }
        case icmp_is_override_checksum:
        {
            bool ovr = value.toBool();
            data.set_is_override_checksum(ovr);
            isOk = true;
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

QWidget* IcmpProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new IcmpConfigForm;
        loadConfigWidget();
    }

    return configForm;
}

void IcmpProtocol::loadConfigWidget()
{
    configWidget();

    configForm->typeCombo->setValue(fieldData(icmp_type, FieldValue).toUInt());
    configForm->codeEdit->setText(fieldData(icmp_code, FieldValue).toString());

    configForm->overrideCksum->setChecked(
        fieldData(icmp_is_override_checksum, FieldValue).toBool());
    configForm->cksumEdit->setText(uintToHexStr(
        fieldData(icmp_checksum, FieldValue).toUInt(), 2));

    configForm->idEdit->setText(
            fieldData(icmp_identifier, FieldValue).toString());
    configForm->seqEdit->setText(
            fieldData(icmp_sequence, FieldValue).toString());

}

void IcmpProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();
    setFieldData(icmp_type, configForm->typeCombo->currentValue());
    setFieldData(icmp_code, configForm->codeEdit->text());

    setFieldData(icmp_is_override_checksum, 
        configForm->overrideCksum->isChecked());
    setFieldData(icmp_checksum, configForm->cksumEdit->text().toUInt(&isOk, BASE_HEX));

    setFieldData(icmp_identifier, configForm->idEdit->text());
    setFieldData(icmp_sequence, configForm->seqEdit->text());
}

