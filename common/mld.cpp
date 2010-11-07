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

#include "mld.h"

#include "ipv6addressvalidator.h"

#include <QHostAddress>
#include <QItemDelegate>
#include <qendian.h>

class IpAddressDelegate : public QItemDelegate
{
public:
    IpAddressDelegate(QObject *parent = 0)
        : QItemDelegate(parent) { }
    ~IpAddressDelegate() {}
    QWidget* createEditor(QWidget *parent, 
            const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QLineEdit *ipEdit;

        ipEdit = static_cast<QLineEdit*>(QItemDelegate::createEditor(
                    parent, option, index));

        // FIXME: const problem!!!
        //ipEdit->setValidator(new IPv6AddressValidator(this));

        return ipEdit;
    }
};

MldConfigForm::MldConfigForm(QWidget *parent)
    : GmpConfigForm(parent)
{
    connect(msgTypeCombo, SIGNAL(currentIndexChanged(int)),
            SLOT(on_msgTypeCombo_currentIndexChanged(int)));

    _defaultSourceIp = "::";
    sourceList->setItemDelegate(new IpAddressDelegate(this));
    groupRecordSourceList->setItemDelegate(new IpAddressDelegate(this));
}

void MldConfigForm::on_msgTypeCombo_currentIndexChanged(int /*index*/)
{
    switch(msgTypeCombo->currentValue())
    {
    case kMldV1Query:
    case kMldV1Report:
    case kMldV1Done:
        asmGroup->show();
        ssmWidget->hide();
        break;

    case kMldV2Query:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(kSsmQueryPage);
        ssmWidget->show();
        break;

    case kMldV2Report:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(kSsmReportPage);
        ssmWidget->show();
        break;

    default:
        asmGroup->hide();
        ssmWidget->hide();
        break;
    }
}

MldProtocol::MldProtocol(StreamBase *stream, AbstractProtocol *parent)
    : GmpProtocol(stream, parent)
{
    data.set_type(kMldV1Query);
}

MldProtocol::~MldProtocol()
{
}

AbstractProtocol* MldProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new MldProtocol(stream, parent);
}

quint32 MldProtocol::protocolNumber() const
{
    return OstProto::Protocol::kMldFieldNumber;
}

void MldProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::mld)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void MldProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::mld))
        data.MergeFrom(protocol.GetExtension(OstProto::mld));
}

QString MldProtocol::name() const
{
    return QString("Multicast Listener Discovery");
}

QString MldProtocol::shortName() const
{
    return QString("MLD");
}

quint32 MldProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x3a;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

AbstractProtocol::FieldFlags MldProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = GmpProtocol::fieldFlags(index);

    switch(index)
    {
        case kMldMrt:
        case kMldRsvd:
            if (msgType() != kMldV2Report)
                flags |= FrameField;
            break;
        default:
            break;
    }

    return flags;
}

QVariant MldProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case kRsvdMrtCode:
        {
            switch(attrib)
            {
            case FieldName: return QString("Code");
            default: break;
            }
            break;
        }

        case kMldMrt:
        {
            quint16 mrt = 0, mrc;

            if (msgType() == kMldV2Query)
            {
                mrt = data.max_response_time();  
                mrc = mrt; // TODO: MR Code
            }
            if (msgType() == kMldV1Query)
                mrc = mrt = data.max_response_time() & 0xFFFF;

            switch(attrib)
            {
            case FieldName:
                if (isQuery())
                    return QString("Max Response Time");
                return QString("Reserved");
            case FieldValue:
                return mrt;
            case FieldTextValue:
                return QString("%1 ms").arg(mrt);
            case FieldFrameValue:
            {
                QByteArray fv;

                fv.resize(2);
                qToBigEndian(mrc, (uchar*) fv.data());
                return fv;
            }
            default:
                break;
            }
            break;
        }
        case kGroupAddress:
        {
            quint64 grpHi, grpLo;
#if 0 
            AbstractProtocol::getip(
                    data.group_address().v6_hi(),
                    data.group_address().v6_lo(),
                    data.group_mode(),
                    data.group_count(),
                    data.group_prefix(),
                    &grpHi, 
                    &grpLo);
#endif

            switch(attrib)
            {
            case FieldName:            
                return QString("Group Address");
            case FieldValue:
            case FieldFrameValue:
            case FieldTextValue:
            {
                QByteArray fv;
                fv.resize(16);
                qToBigEndian(grpHi, (uchar*) fv.data());
                qToBigEndian(grpLo, (uchar*) (fv.data() + 8));
                if (attrib == FieldFrameValue)
                    return fv;
                else
                    return QHostAddress((quint8*)fv.constData()).toString();
            }
            default:
                break;
            }
            break;
        }
        case kSources:
        {
            quint64 grpHi, grpLo;
            
            switch(attrib)
            {
            case FieldName:            
                return QString("Source List");
            case FieldValue:
                return QVariant(); // FIXME
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(16 * data.sources_size());
                for (int i = 0; i < data.sources_size(); i++)
                {
                    qToBigEndian(grpHi, (uchar*)(fv.data() + i*16));
                    qToBigEndian(grpLo, (uchar*)(fv.data() + i*16 + 8));
                }
                return fv;
            }
            case FieldTextValue:
            {
                QStringList list;
                QByteArray fv;
                fv.resize(16);
                for (int i = 0; i < data.sources_size(); i++)
                {
                    qToBigEndian(grpHi, (uchar*)fv.data());
                    qToBigEndian(grpLo, (uchar*)fv.data()+8);

                    list << QHostAddress((quint8*)fv.constData()).toString();
                }
                return list.join(", ");
            }
            default:
                break;
            }
            break;
        }
        case kGroupRecords:
            // TODO
            break;
        default:
            break;
    }

    return GmpProtocol::fieldData(index, attrib, streamIndex);
}

bool MldProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case kGroupAddress:
        {
            Q_IPV6ADDR addr = QHostAddress(value.toString()).toIPv6Address();
            quint64 x;

            x =   (quint64(addr[0]) << 56)
                | (quint64(addr[1]) << 48)
                | (quint64(addr[2]) << 40)
                | (quint64(addr[3]) << 32)
                | (quint64(addr[4]) << 24)
                | (quint64(addr[5]) << 16)
                | (quint64(addr[6]) <<  8)
                | (quint64(addr[7]) <<  0);
            data.mutable_group_address()->set_v6_hi(x);

            x =   (quint64(addr[ 8]) << 56)
                | (quint64(addr[ 9]) << 48)
                | (quint64(addr[10]) << 40)
                | (quint64(addr[11]) << 32)
                | (quint64(addr[12]) << 24)
                | (quint64(addr[13]) << 16)
                | (quint64(addr[14]) <<  8)
                | (quint64(addr[15]) <<  0);
            data.mutable_group_address()->set_v6_lo(x);
            break;
        }

        case kSources:
            //TODO
            break;

        case kGroupRecords:
            //TODO
            break;

        default:
            isOk = GmpProtocol::setFieldData(index, value, attrib);
            break;
    }

_exit:
    return isOk;
}

QWidget* MldProtocol::configWidget()
{
    /* Lazy creation of the configWidget */
    if (configForm == NULL)
    {
        configForm = new MldConfigForm;
        loadConfigWidget();
    }

    return configForm;
}

void MldProtocol::loadConfigWidget()
{
    GmpProtocol::loadConfigWidget();

    configForm->maxResponseTime->setText(
            fieldData(kMldMrt, FieldValue).toString());
#if 0
    configForm->mldA->setText(fieldData(mld_a, FieldValue).toString());
    configForm->mldB->setText(fieldData(mld_b, FieldValue).toString());

    configForm->mldPayloadLength->setText(
        fieldData(mld_payloadLength, FieldValue).toString());

    configForm->isChecksumOverride->setChecked(
        fieldData(mld_is_override_checksum, FieldValue).toBool());
    configForm->mldChecksum->setText(uintToHexStr(
        fieldData(mld_checksum, FieldValue).toUInt(), 2));

    configForm->mldX->setText(fieldData(mld_x, FieldValue).toString());
    configForm->mldY->setText(fieldData(mld_y, FieldValue).toString());
#endif
}

void MldProtocol::storeConfigWidget()
{
    bool isOk;

    GmpProtocol::storeConfigWidget();

#if 0
    setFieldData(mld_a, configForm->mldA->text());
    setFieldData(mld_b, configForm->mldB->text());

    setFieldData(mld_payloadLength, configForm->mldPayloadLength->text());
    setFieldData(mld_is_override_checksum, 
        configForm->isChecksumOverride->isChecked());
    setFieldData(mld_checksum, configForm->mldChecksum->text().toUInt(&isOk, BASE_HEX));

    setFieldData(mld_x, configForm->mldX->text());
    setFieldData(mld_y, configForm->mldY->text());
#endif
}

quint16 MldProtocol::checksum(int streamIndex) const
{
    return AbstractProtocol::protocolFrameCksum(streamIndex, CksumTcpUdp);
}
