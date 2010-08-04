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

#include "igmp.h"

#include <QHostAddress>
#include <qendian.h>


IgmpConfigForm::IgmpConfigForm(QWidget *parent)
    : GmpConfigForm(parent)
{
}

IgmpProtocol::IgmpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : GmpProtocol(stream, parent)
{
    data.set_type(kIgmpV2Query);
}

IgmpProtocol::~IgmpProtocol()
{
}

AbstractProtocol* IgmpProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new IgmpProtocol(stream, parent);
}

quint32 IgmpProtocol::protocolNumber() const
{
    return OstProto::Protocol::kIgmpFieldNumber;
}

void IgmpProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::igmp)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void IgmpProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::igmp))
        data.MergeFrom(protocol.GetExtension(OstProto::igmp));
}

QString IgmpProtocol::name() const
{
    return QString("Internet Group Management Protocol");
}

QString IgmpProtocol::shortName() const
{
    return QString("IGMP");
}

quint32 IgmpProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdIp: return 0x2;
        default:break;
    }

    return AbstractProtocol::protocolId(type);
}

QVariant IgmpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case kRsvdMrtCode:
        {
            quint8 mrt = 0, mrc;

            if (msgType() == kIgmpV3Query)
            {
                mrt = data.max_response_time();
                mrc = mrt;  // TODO: MR Code
            }
            else if (msgType() == kIgmpV2Query)
                mrc = mrt = data.max_response_time() & 0xFF;


            switch(attrib)
            {
            case FieldName:
                if (isQuery())
                    return QString("Max Response Time");
                else
                    return QString("Reserved");
            case FieldValue:
                return mrt;
            case FieldTextValue:
                return QString("%1 ms").arg(mrt);
            case FieldFrameValue:
                return QByteArray(1, mrc);
            default:
                break;
            }
            break;
        }
        case kGroupAddress:
        {
            quint32 grpIp = data.group_address().v4();  // FIXME
#if 0 // TODO
            getip(
                    data.group_address().v4(),
                    data.group_mode(),
                    data.group_count(),
                    data.group_prefix());
#endif

            switch(attrib)
            {
            case FieldName:            
                return QString("Group Address");
            case FieldValue:
            case FieldTextValue:
                return QHostAddress(grpIp).toString();
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(4);
                qToBigEndian(grpIp, (uchar*) fv.data());
                return fv;
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
                return QVariant(); // FIXME
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(4 * data.sources_size());
                for (int i = 0; i < data.sources_size(); i++)
                    qToBigEndian(data.sources(i), (uchar*) (fv.data()+4*i));
                return fv;
            }
            case FieldTextValue:
            {
                QStringList list;

                for (int i = 0; i < data.sources_size(); i++)
                    list.append(QHostAddress(data.sources(i).v4()).toString());
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
            case FieldName:            
                return QString("Group List");
            case FieldValue:
                return QVariant(); // FIXME
            case FieldFrameValue:
            {
                QByteArray fv;
                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QByteArray rv;

                    rv.resize(4 + 4 + 4*data.group_records(i).sources_size() 
                            + data.group_records(i).aux_data().size());
                    rv[0] = rec.type();
                    rv[1] = rec.is_override_aux_data_length() ? 
                        rec.aux_data_length() : rec.aux_data().size();
                    if (rec.is_override_source_count())
                        qToBigEndian(rec.source_count(),(uchar*)(rv.data()+2));
                    else
                        qToBigEndian(rec.sources_size(),(uchar*)(rv.data()+2));
                    qToBigEndian(rec.group_address().v4(), 
                            (uchar*)(fv.data()+4));
                    for (int j = 0; j < rec.sources_size(); j++)
                        qToBigEndian(rec.sources(j),(uchar*)(rv.data()+8+4*j));
                    rv.append(QString().fromStdString(rec.aux_data()).toUtf8());

                    fv.append(rv);
                }
                return fv;
            }
            case FieldTextValue:
            {
                QStringList list;

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QString str;

                    str.append("Type: ");
                    switch(rec.type())
                    {
                    case OstProto::Gmp::GroupRecord::kIsInclude: 
                        str.append("IS_INCLUDE"); break;
                    case OstProto::Gmp::GroupRecord::kIsExclude: 
                        str.append("IS_EXCLUDE"); break;
                    case OstProto::Gmp::GroupRecord::kToInclude: 
                        str.append("TO_INCLUDE"); break;
                    case OstProto::Gmp::GroupRecord::kToExclude: 
                        str.append("TO_EXCLUDE"); break;
                    case OstProto::Gmp::GroupRecord::kAllowNew: 
                        str.append("ALLOW_NEW"); break;
                    case OstProto::Gmp::GroupRecord::kBlockOld: 
                        str.append("BLOCK_OLD"); break;
                    default: 
                        str.append("UNKNOWN"); break;
                    }
                    str.append(QString("; AuxLen: %1").arg(
                        rec.is_override_aux_data_length() ? 
                            rec.aux_data_length() : rec.aux_data().size()));
                    str.append(QString("; Source Count: %1").arg(
                        rec.is_override_source_count() ?
                            rec.source_count(): rec.sources_size()));
                    str.append(QString("; Group: %1").arg(
                        QHostAddress(rec.group_address().v4()).toString()));

                    str.append("; Sources: ");
                    QStringList l;
                    for (int j = 0; j < rec.sources_size(); j++)
                        l.append(QHostAddress(rec.sources(j).v4()).toString());
                    str.append(l.join(", "));
                    str.append(QString().fromStdString(rec.aux_data()));

                    list.append(str);
                }
                return list.join("\n");
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

bool IgmpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case kGroupAddress:
        {
            QHostAddress addr(value.toString());
            quint32 ip = addr.toIPv4Address();
            isOk = (addr.protocol() == QAbstractSocket::IPv4Protocol);
            if (isOk)
                data.mutable_group_address()->set_v4(ip);
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

void IgmpProtocol::loadConfigWidget()
{
    GmpProtocol::loadConfigWidget();

    configForm->maxResponseTime->setText(
            fieldData(kRsvdMrtCode, FieldValue).toString());
#if 0
    configForm->igmpA->setText(fieldData(igmp_a, FieldValue).toString());
    configForm->igmpB->setText(fieldData(igmp_b, FieldValue).toString());

    configForm->igmpPayloadLength->setText(
        fieldData(igmp_payloadLength, FieldValue).toString());

    configForm->isChecksumOverride->setChecked(
        fieldData(igmp_is_override_checksum, FieldValue).toBool());
    configForm->igmpChecksum->setText(uintToHexStr(
        fieldData(igmp_checksum, FieldValue).toUInt(), 2));

    configForm->igmpX->setText(fieldData(igmp_x, FieldValue).toString());
    configForm->igmpY->setText(fieldData(igmp_y, FieldValue).toString());
#endif
}

void IgmpProtocol::storeConfigWidget()
{
    bool isOk;

    GmpProtocol::storeConfigWidget();

#if 0
    setFieldData(igmp_a, configForm->igmpA->text());
    setFieldData(igmp_b, configForm->igmpB->text());

    setFieldData(igmp_payloadLength, configForm->igmpPayloadLength->text());
    setFieldData(igmp_is_override_checksum, 
        configForm->isChecksumOverride->isChecked());
    setFieldData(igmp_checksum, configForm->igmpChecksum->text().toUInt(&isOk, BASE_HEX));

    setFieldData(igmp_x, configForm->igmpX->text());
    setFieldData(igmp_y, configForm->igmpY->text());
#endif
}

quint16 IgmpProtocol::checksum(int streamIndex) const
{
    quint16 cks;
    quint32 sum = 0;
#if 0 // FIXME
    // TODO: add as a new CksumType (CksumIgmp?) and implement in AbsProto 
    cks = protocolFrameCksum(streamIndex, CksumIp);
    sum += (quint16) ~cks;
    cks = protocolFramePayloadCksum(streamIndex, CksumIp);
    sum += (quint16) ~cks;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    cks = (~sum) & 0xFFFF;
#endif
    return cks;
}
