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

#include "ipv6addressdelegate.h"
#include "ipv6addressvalidator.h"
#include "iputils.h"

#include <QHostAddress>
#include <qendian.h>

MldConfigForm::MldConfigForm(QWidget *parent)
    : GmpConfigForm(parent)
{
    connect(msgTypeCombo, SIGNAL(currentIndexChanged(int)),
            SLOT(on_msgTypeCombo_currentIndexChanged(int)));

    msgTypeCombo->setValueMask(0xFF);
    msgTypeCombo->addItem(kMldV1Query,  "MLDv1 Query");
    msgTypeCombo->addItem(kMldV1Report, "MLDv1 Report");
    msgTypeCombo->addItem(kMldV1Done,   "MLDv1 Done");
    msgTypeCombo->addItem(kMldV2Query,  "MLDv2 Query");
    msgTypeCombo->addItem(kMldV2Report, "MLDv2 Report");

    _defaultGroupIp  = "::";
    _defaultSourceIp = "::";

    groupAddress->setValidator(new IPv6AddressValidator(this));
    groupRecordAddress->setValidator(new IPv6AddressValidator(this));
    sourceList->setItemDelegate(new IPv6AddressDelegate(this));
    groupRecordSourceList->setItemDelegate(new IPv6AddressDelegate(this));
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
    _hasPayload = false;

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
            quint16 mrt = 0, mrcode = 0;

            if (msgType() == kMldV2Query)
            {
                mrt = data.max_response_time();  
                mrcode = mrc(mrt);
            }
            else if (msgType() == kMldV1Query)
                mrcode = mrt = data.max_response_time() & 0xFFFF;

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
                qToBigEndian(mrcode, (uchar*) fv.data());
                return fv;
            }
            default:
                break;
            }
            break;
        }
        case kMldRsvd:
        {
            quint16 rsvd = 0;

            switch(attrib)
            {
            case FieldName:
                return QString("Reserved");
            case FieldValue:
                return rsvd;
            case FieldTextValue:
                return QString("%1").arg(rsvd);
            case FieldFrameValue:
            {
                QByteArray fv;

                fv.resize(2);
                qToBigEndian(rsvd, (uchar*) fv.data());
                return fv;
            }
            default:
                break;
            }
            break;
        }
        case kGroupAddress:
        {
            quint64 grpHi = 0, grpLo = 0;

            ipUtils::ipAddress(
                    data.group_address().v6_hi(),
                    data.group_address().v6_lo(),
                    data.group_prefix(),
                    ipUtils::AddrMode(data.group_mode()),
                    data.group_count(),
                    streamIndex,
                    grpHi, 
                    grpLo);

            switch(attrib)
            {
            case FieldName:            
                return QString("Group Address");
            case FieldValue:
            case FieldTextValue:
            case FieldFrameValue:
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
            switch(attrib)
            {
            case FieldName:            
                return QString("Source List");
            case FieldValue:
            {
                QStringList list;
                QByteArray fv;
                fv.resize(16);
                for (int i = 0; i < data.sources_size(); i++)
                {
                    qToBigEndian(data.sources(i).v6_hi(), 
                            (uchar*)fv.data());
                    qToBigEndian(data.sources(i).v6_lo(),
                            (uchar*)fv.data()+8);

                    list << QHostAddress((quint8*)fv.constData()).toString();
                }
                return list;
            }
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(16 * data.sources_size());
                for (int i = 0; i < data.sources_size(); i++)
                {
                    qToBigEndian(data.sources(i).v6_hi(), 
                            (uchar*)(fv.data() + i*16));
                    qToBigEndian(data.sources(i).v6_lo(),
                            (uchar*)(fv.data() + i*16 + 8));
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
                    qToBigEndian(data.sources(i).v6_hi(), 
                            (uchar*)fv.data());
                    qToBigEndian(data.sources(i).v6_lo(),
                            (uchar*)fv.data()+8);

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
        {
            switch(attrib)
            {
            case FieldValue:
            {
                QVariantList grpRecords = GmpProtocol::fieldData(
                        index, attrib, streamIndex).toList();
                QByteArray ip;

                ip.resize(16);

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    QVariantMap grpRec = grpRecords.at(i).toMap();
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);

                    qToBigEndian(quint64(rec.group_address().v6_hi()), 
                            (uchar*)(ip.data()));
                    qToBigEndian(quint64(rec.group_address().v6_lo()),
                            (uchar*)(ip.data() + 8));
                    grpRec["groupRecordAddress"] = QHostAddress(
                            (quint8*)ip.constData()).toString();

                    QStringList sl;
                    for (int j = 0; j < rec.sources_size(); j++)
                    {
                        qToBigEndian(rec.sources(j).v6_hi(), 
                                (uchar*)(ip.data()));
                        qToBigEndian(rec.sources(j).v6_lo(),
                                (uchar*)(ip.data() + 8));
                        sl.append(QHostAddress(
                                (quint8*)ip.constData()).toString());
                    }
                    grpRec["groupRecordSourceList"] = sl;

                    grpRecords.replace(i, grpRec);
                }
                return grpRecords;
            }
            case FieldFrameValue:
            {
                QVariantList list = GmpProtocol::fieldData(
                        index, attrib, streamIndex).toList();
                QByteArray fv;
                QByteArray ip;
                ip.resize(16);

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QByteArray rv = list.at(i).toByteArray();

                    rv.insert(4, QByteArray(16+16*rec.sources_size(), char(0)));
                    qToBigEndian(rec.group_address().v6_hi(), 
                            (uchar*)(rv.data()+4));
                    qToBigEndian(rec.group_address().v6_lo(), 
                            (uchar*)(rv.data()+4+8));
                    for (int j = 0; j < rec.sources_size(); j++)
                    {
                        qToBigEndian(rec.sources(j).v6_hi(),
                                (uchar*)(rv.data()+20+16*j));
                        qToBigEndian(rec.sources(j).v6_lo(),
                                (uchar*)(rv.data()+20+16*j+8));
                    }

                    fv.append(rv);
                }
                return fv;
            }
            case FieldTextValue:
            {
                QStringList list = GmpProtocol::fieldData(
                        index, attrib, streamIndex).toStringList();
                QByteArray ip;

                ip.resize(16);

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QString recStr = list.at(i);
                    QString str;

                    qToBigEndian(rec.group_address().v6_hi(), 
                            (uchar*)(ip.data()));
                    qToBigEndian(rec.group_address().v6_lo(),
                            (uchar*)(ip.data() + 8));
                    str.append(QString("Group: %1").arg(
                        QHostAddress((quint8*)ip.constData()).toString()));

                    str.append("; Sources: ");
                    QStringList sl;
                    for (int j = 0; j < rec.sources_size(); j++)
                    {
                        qToBigEndian(rec.sources(j).v6_hi(), 
                                (uchar*)(ip.data()));
                        qToBigEndian(rec.sources(j).v6_lo(),
                                (uchar*)(ip.data() + 8));
                        sl.append(QHostAddress(
                                (quint8*)ip.constData()).toString());
                    }
                    str.append(sl.join(", "));

                    recStr.replace("XXX", str);
                    list.replace(i, recStr);
                }
                return list.join("\n").insert(0, "\n");
            }
            default:
                break;
            }
            break;
        }
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
        {
            QStringList list = value.toStringList();

            data.clear_sources();
            foreach(QString str, list)
            {
                OstProto::Gmp::IpAddress *src = data.add_sources();
                Q_IPV6ADDR addr = QHostAddress(str).toIPv6Address();
                quint64 x;

                x =   (quint64(addr[0]) << 56)
                    | (quint64(addr[1]) << 48)
                    | (quint64(addr[2]) << 40)
                    | (quint64(addr[3]) << 32)
                    | (quint64(addr[4]) << 24)
                    | (quint64(addr[5]) << 16)
                    | (quint64(addr[6]) <<  8)
                    | (quint64(addr[7]) <<  0);
                src->set_v6_hi(x);

                x =   (quint64(addr[ 8]) << 56)
                    | (quint64(addr[ 9]) << 48)
                    | (quint64(addr[10]) << 40)
                    | (quint64(addr[11]) << 32)
                    | (quint64(addr[12]) << 24)
                    | (quint64(addr[13]) << 16)
                    | (quint64(addr[14]) <<  8)
                    | (quint64(addr[15]) <<  0);
                src->set_v6_lo(x);
            }
            break;
        }

        case kGroupRecords:
        {
            GmpProtocol::setFieldData(index, value, attrib);
            QVariantList list = value.toList();

            for (int i = 0; i < list.count(); i++)
            {
                QVariantMap grpRec = list.at(i).toMap();
                OstProto::Gmp::GroupRecord *rec = data.mutable_group_records(i);
                Q_IPV6ADDR addr = QHostAddress(
                        grpRec["groupRecordAddress"].toString())
                        .toIPv6Address();
                quint64 x;

                x =   (quint64(addr[0]) << 56)
                    | (quint64(addr[1]) << 48)
                    | (quint64(addr[2]) << 40)
                    | (quint64(addr[3]) << 32)
                    | (quint64(addr[4]) << 24)
                    | (quint64(addr[5]) << 16)
                    | (quint64(addr[6]) <<  8)
                    | (quint64(addr[7]) <<  0);
                rec->mutable_group_address()->set_v6_hi(x);

                x =   (quint64(addr[ 8]) << 56)
                    | (quint64(addr[ 9]) << 48)
                    | (quint64(addr[10]) << 40)
                    | (quint64(addr[11]) << 32)
                    | (quint64(addr[12]) << 24)
                    | (quint64(addr[13]) << 16)
                    | (quint64(addr[14]) <<  8)
                    | (quint64(addr[15]) <<  0);
                rec->mutable_group_address()->set_v6_lo(x);
                
                QStringList srcList = grpRec["groupRecordSourceList"]
                                            .toStringList();
                rec->clear_sources();
                foreach (QString str, srcList)
                {
                    OstProto::Gmp::IpAddress *src = rec->add_sources();
                    Q_IPV6ADDR addr = QHostAddress(str).toIPv6Address();
                    quint64 x;

                    x =   (quint64(addr[0]) << 56)
                        | (quint64(addr[1]) << 48)
                        | (quint64(addr[2]) << 40)
                        | (quint64(addr[3]) << 32)
                        | (quint64(addr[4]) << 24)
                        | (quint64(addr[5]) << 16)
                        | (quint64(addr[6]) <<  8)
                        | (quint64(addr[7]) <<  0);
                    src->set_v6_hi(x);

                    x =   (quint64(addr[ 8]) << 56)
                        | (quint64(addr[ 9]) << 48)
                        | (quint64(addr[10]) << 40)
                        | (quint64(addr[11]) << 32)
                        | (quint64(addr[12]) << 24)
                        | (quint64(addr[13]) << 16)
                        | (quint64(addr[14]) <<  8)
                        | (quint64(addr[15]) <<  0);
                    src->set_v6_lo(x);
                }
            }

            break;
        }

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
}

void MldProtocol::storeConfigWidget()
{
    GmpProtocol::storeConfigWidget();

    setFieldData(kMldMrt, configForm->maxResponseTime->text());
}

quint16 MldProtocol::checksum(int streamIndex) const
{
    return AbstractProtocol::protocolFrameCksum(streamIndex, CksumTcpUdp);
}
