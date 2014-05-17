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
#include "iputils.h"

#include <QHostAddress>
#include <QStringList>

IgmpProtocol::IgmpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : GmpProtocol(stream, parent)
{
    _hasPayload = false;

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
            uint mrt = 0;
            quint8 mrcode = 0;

            if (msgType() == kIgmpV3Query)
            {
                mrt = data.max_response_time();
                mrcode = quint8(mrc(mrt));
            }
            else if (msgType() == kIgmpV2Query)
            {
                mrt = data.max_response_time();
                mrcode = mrt & 0xFF;
            }


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
                return QString("%1").arg(mrt);
            case FieldFrameValue:
                return QByteArray(1, mrcode);
            default:
                break;
            }
            break;
        }
        case kGroupAddress:
        {
            quint32 grpIp = ipUtils::ipAddress(
                data.group_address().v4(),
                data.group_prefix(),
                ipUtils::AddrMode(data.group_mode()),
                data.group_count(),
                streamIndex);

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
            {
                QStringList list;

                for (int i = 0; i < data.sources_size(); i++)
                    list.append(QHostAddress(data.sources(i).v4()).toString());
                return list;
            }
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(4 * data.sources_size());
                for (int i = 0; i < data.sources_size(); i++)
                    qToBigEndian(data.sources(i).v4(), (uchar*)(fv.data()+4*i));
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
            case FieldValue:
            {
                QVariantList grpRecords = GmpProtocol::fieldData(
                        index, attrib, streamIndex).toList();

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    QVariantMap grpRec = grpRecords.at(i).toMap();
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);

                    grpRec["groupRecordAddress"] = QHostAddress(
                                rec.group_address().v4()).toString();

                    QStringList sl;
                    for (int j = 0; j < rec.sources_size(); j++)
                        sl.append(QHostAddress(rec.sources(j).v4()).toString());
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

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QByteArray rv = list.at(i).toByteArray();

                    rv.insert(4, QByteArray(4+4*rec.sources_size(), char(0)));
                    qToBigEndian(rec.group_address().v4(), 
                            (uchar*)(rv.data()+4));
                    for (int j = 0; j < rec.sources_size(); j++)
                    {
                        qToBigEndian(rec.sources(j).v4(),
                                (uchar*)(rv.data()+8+4*j));
                    }

                    fv.append(rv);
                }
                return fv;
            }
            case FieldTextValue:
            {
                QStringList list = GmpProtocol::fieldData(
                        index, attrib, streamIndex).toStringList();

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QString recStr = list.at(i);
                    QString str;

                    str.append(QString("Group: %1").arg(
                        QHostAddress(rec.group_address().v4()).toString()));

                    str.append("; Sources: ");
                    QStringList sl;
                    for (int j = 0; j < rec.sources_size(); j++)
                        sl.append(QHostAddress(rec.sources(j).v4()).toString());
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

bool IgmpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case kRsvdMrtCode:
        {
            uint mrt = value.toUInt(&isOk);
            if (isOk)
                data.set_max_response_time(mrt);
            break;
        }
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
        {
            QStringList list = value.toStringList();

            data.clear_sources();
            foreach(QString str, list)
            {
                quint32 ip = QHostAddress(str).toIPv4Address();
                data.add_sources()->set_v4(ip);
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
                
                rec->mutable_group_address()->set_v4(QHostAddress(
                            grpRec["groupRecordAddress"].toString())
                            .toIPv4Address());

                QStringList srcList = grpRec["groupRecordSourceList"]
                                            .toStringList();
                rec->clear_sources();
                foreach (QString src, srcList)
                {
                    rec->add_sources()->set_v4(
                            QHostAddress(src).toIPv4Address());
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

quint16 IgmpProtocol::checksum(int streamIndex) const
{
    quint16 cks;
    quint32 sum = 0;

    // TODO: add as a new CksumType (CksumIgmp?) and implement in AbsProto 
    cks = protocolFrameCksum(streamIndex, CksumIp);
    sum += (quint16) ~cks;
    cks = protocolFramePayloadCksum(streamIndex, CksumIp);
    sum += (quint16) ~cks;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    cks = (~sum) & 0xFFFF;

    return cks;
}
