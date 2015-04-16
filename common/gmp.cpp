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

#include "gmp.h"
#include <QStringList>

QHash<int, int> GmpProtocol::frameFieldCountMap;

GmpProtocol::GmpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
}

GmpProtocol::~GmpProtocol()
{
}

AbstractProtocol::ProtocolIdType GmpProtocol::protocolIdType() const
{
    return ProtocolIdIp;
}

int GmpProtocol::fieldCount() const
{
    return FIELD_COUNT;
}

int GmpProtocol::frameFieldCount() const
{
    int type = msgType();

    // frameFieldCountMap contains the frameFieldCounts for each
    // msgType - this is built on demand and cached for subsequent use

    // lookup if we have already cached ...
    if (frameFieldCountMap.contains(type))
        return frameFieldCountMap.value(type);

    // ... otherwise calculate and cache 
    int count = 0;
    for (int i = 0; i < FIELD_COUNT; i++)
    {
        if (fieldFlags(i).testFlag(AbstractProtocol::FrameField))
            count++;
    }
    frameFieldCountMap.insert(type, count);
    return count;
}

AbstractProtocol::FieldFlags GmpProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);
    flags &= ~FrameField;

    switch(index)
    {
        // Frame Fields - check against msgType()
        case kType:
        case kRsvdMrtCode:
            flags |= FrameField;
            break;
        case kChecksum:
            flags |= FrameField;
            flags |= CksumField;
            break;
        case kMldMrt:
        case kMldRsvd:
            // MLD subclass should handle suitably 
            break;

        case kGroupAddress:
            if (!isSsmReport())
                flags |= FrameField;
            break;

        case kRsvd1:
        case kSFlag:
        case kQrv:
        case kQqic:
        case kSourceCount:
        case kSources:
            if (isSsmQuery())
                flags |= FrameField;
            break;

        case kRsvd2:
        case kGroupRecordCount:
        case kGroupRecords:
            if (isSsmReport())
                flags |= FrameField;
            break;

        // Meta Fields
        case kIsOverrideChecksum:
        case kGroupMode:
        case kGroupCount:
        case kGroupPrefix:
        case kIsOverrideSourceCount:
        case kIsOverrideGroupRecordCount:
            flags |= MetaField;
            break;

        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

    return flags;
}

QVariant GmpProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case kType:
        {
            uint type = data.type();

            switch(attrib)
            {
            case FieldName:            
                return QString("Type");
            case FieldValue:
                return type;
            case FieldTextValue:
                return QString("%1").arg(quint8(type));
            case FieldFrameValue:
                return QByteArray(1, quint8(type));
            default:
                break;
            }
            break;
        }
        case kRsvdMrtCode:
        {
            quint8 rsvd = 0;

            switch(attrib)
            {
            case FieldName:
                return QString("Reserved");
            case FieldValue:
                return rsvd;
            case FieldTextValue:
                return QString("%1").arg(rsvd);
            case FieldFrameValue:
                return QByteArray(1, rsvd);
            default:
                break;
            }
            break;
        }
        case kChecksum:
        {
            switch(attrib)
            {
            case FieldName:            
                return QString("Checksum");
            case FieldBitSize:
                return 16;
            default:
                break;
            }

            quint16 cksum = data.is_override_checksum() ?  
                data.checksum() : checksum(streamIndex); 

            switch(attrib)
            {
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
                return QString("0x%1").arg(cksum, 4, BASE_HEX, QChar('0'));
            default:
                break;
            }
            break;
        }
        case kMldMrt:
        case kMldRsvd:
            // XXX: Present only in MLD - hence handled by the mld subclass
            break;

        case kGroupAddress:
            // XXX: Handled by each subclass
            break;

        case kRsvd1:
        {
            quint8 rsvd = 0;

            switch(attrib)
            {
            case FieldName:            
                return QString("Reserved");
            case FieldValue:
                return rsvd;
            case FieldTextValue:
                return QString("%1").arg(rsvd);
            case FieldFrameValue:
                return QByteArray(1, char(rsvd));
            case FieldBitSize:
                return 4;
            default:
                break;
            }
            break;
        }
        case kSFlag:
        {
            switch(attrib)
            {
            case FieldName:            
                return QString("S Flag");
            case FieldValue:
                return data.s_flag();
            case FieldTextValue:
                return data.s_flag() ? QString("True") : QString("False");
            case FieldFrameValue:
                return QByteArray(1, char(data.s_flag()));
            case FieldBitSize:
                return 1;
            default:
                break;
            }
            break;
        }
        case kQrv:
        {
            int qrv = data.qrv() & 0x7;

            switch(attrib)
            {
            case FieldName:            
                return QString("QRV");
            case FieldValue:
                return qrv;
            case FieldTextValue:
                return QString("%1").arg(qrv);
            case FieldFrameValue:
                return QByteArray(1, char(qrv));
            case FieldBitSize:
                return 3;
            default:
                break;
            }
            break;
        }
        case kQqic:
        {
            int qqi = data.qqi();

            switch(attrib)
            {
                case FieldName:            
                    return QString("QQIC");
                case FieldValue:
                    return qqi;
                case FieldTextValue:
                    return QString("%1").arg(qqi);
                case FieldFrameValue:
                {
                    char qqicode = char(qqic(qqi));
                    return QByteArray(1, qqicode);
                }
                default:
                    break;
            }
            break;
        }
        case kSourceCount:
        {
            quint16 count = data.sources_size();

            if (data.is_override_source_count())
                count = data.source_count();

            switch(attrib)
            {
            case FieldName:            
                return QString("Number of Sources");
            case FieldValue:
                return count;
            case FieldTextValue:
                return QString("%1").arg(count);
            case FieldFrameValue:
            {
                QByteArray fv;
                fv.resize(2);
                qToBigEndian(count, (uchar*) fv.data());
                return fv;
            }
            default:
                break;
            }
            break;
        }
        case kSources:
            // XXX: Handled by each subclass
            break;
        case kRsvd2:
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
        case kGroupRecordCount:
        {
            quint16 count = data.group_records_size();

            if (data.is_override_group_record_count())
                count = data.group_record_count();

            switch(attrib)
            {
                case FieldName:            
                    return QString("Number of Group Records");
                case FieldValue:
                    return count;
                case FieldTextValue:
                    return QString("%1").arg(count);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    qToBigEndian(count, (uchar*) fv.data());
                    return fv;
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
            {
                QVariantList grpRecords;

                for (int i = 0; i < data.group_records_size(); i++)
                {
                    QVariantMap grpRec;
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);

                    grpRec["groupRecordType"] = rec.type();
                    // grpRec["groupRecordAddress"] = subclass responsibility
                    grpRec["overrideGroupRecordSourceCount"] = 
                                rec.is_override_source_count();
                    grpRec["groupRecordSourceCount"] = rec.source_count();

                    // grpRec["groupRecordSourceList"] = subclass responsibility
                    grpRec["overrideAuxDataLength"] = 
                                rec.is_override_aux_data_length();
                    grpRec["auxDataLength"] = rec.aux_data_length();
                    grpRec["auxData"] = QByteArray().append(
                            QString::fromStdString(rec.aux_data()));

                    grpRecords.append(grpRec);
                }
                return grpRecords;
            }
            case FieldFrameValue:
            {
                QVariantList fv;
                for (int i = 0; i < data.group_records_size(); i++)
                {
                    OstProto::Gmp::GroupRecord rec = data.group_records(i);
                    QByteArray rv;
                    quint16 srcCount;

                    rv.resize(4);
                    rv[0] = rec.type();
                    rv[1] = rec.is_override_aux_data_length() ? 
                        rec.aux_data_length() : rec.aux_data().size()/4;

                    if (rec.is_override_source_count())
                        srcCount = rec.source_count();
                    else
                        srcCount = rec.sources_size();
                    qToBigEndian(srcCount, (uchar*)(rv.data()+2));

                    // group_address => subclass responsibility
                    // source list => subclass responsibility

                    rv.append(QString().fromStdString(rec.aux_data()));

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

                    str.append("  Type: ");
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
                            rec.aux_data_length() : rec.aux_data().size()/4));
                    str.append(QString("; Source Count: %1").arg(
                        rec.is_override_source_count() ?
                            rec.source_count(): rec.sources_size()));

                    // NOTE: subclass should replace the XXX below with
                    // group address and source list
                    str.append(QString("; XXX"));

                    str.append(QString("; AuxData: ").append(
                                QByteArray().append(QString().fromStdString(
                                        rec.aux_data())).toHex()));

                    list.append(str);
                }
                return list;
            }
            default:
                break;
            }
            break;
        }

        // Meta Fields
        case kIsOverrideChecksum:
        {
            switch(attrib)
            {
            case FieldValue: return data.is_override_checksum();
            default: break;
            }
            break;
        }
        case kGroupMode:
        {
            switch(attrib)
            {
            case FieldValue: return data.group_mode();
            default: break;
            }
            break;
        }
        case kGroupCount:
        {
            switch(attrib)
            {
            case FieldValue: return data.group_count();
            default: break;
            }
            break;
        }
        case kGroupPrefix:
        {
            switch(attrib)
            {
            case FieldValue: return data.group_prefix();
            default: break;
            }
            break;
        }
        case kIsOverrideSourceCount:
        {
            switch(attrib)
            {
            case FieldValue: return data.is_override_source_count();
            default: break;
            }
            break;
        }
        case kIsOverrideGroupRecordCount:
        {
            switch(attrib)
            {
            case FieldValue: return data.is_override_group_record_count();
            default: break;
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

bool GmpProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case kType:
        {
            uint type = value.toUInt(&isOk);
            if (isOk)
                data.set_type(type);
            break;
        }
        case kRsvdMrtCode:
        {
            uint val = value.toUInt(&isOk);
            if (isOk)
                data.set_rsvd_code(val);
            break;
        }
        case kChecksum:
        {
            uint csum = value.toUInt(&isOk);
            if (isOk)
                data.set_checksum(csum);
            break;
        }
        case kMldMrt:
        {
            uint mrt = value.toUInt(&isOk);
            if (isOk)
                data.set_max_response_time(mrt);
            break;
        }
        case kGroupAddress:
            // XXX: Handled by subclass
            isOk = false;
            break;
        case kRsvd1:
            isOk = false;
            break;
        case kSFlag:
        {
            bool flag = value.toBool();
            data.set_s_flag(flag);
            isOk = true;
            break;
        }
        case kQrv:
        {
            uint qrv = value.toUInt(&isOk);
            if (isOk)
                data.set_qrv(qrv);
            break;
        }
        case kQqic:
        {
            uint qqi = value.toUInt(&isOk);
            if (isOk)
                data.set_qqi(qqi);
            break;
        }
        case kSourceCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_source_count(count);
            break;
        }
        case kSources:
            // XXX: Handled by subclass
            isOk = false;
            break;
        case kRsvd2:
            isOk = false;
            break;
        case kGroupRecordCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_group_record_count(count);
            break;
        }
        case kGroupRecords:
        {
            QVariantList list = value.toList();

            data.clear_group_records();

            for (int i = 0; i < list.count(); i++)
            {
                QVariantMap grpRec = list.at(i).toMap();
                OstProto::Gmp::GroupRecord *rec = data.add_group_records();
                
                rec->set_type(OstProto::Gmp::GroupRecord::RecordType(
                            grpRec["groupRecordType"].toInt()));
                // NOTE: rec->group_address => subclass responsibility
                rec->set_is_override_source_count(
                            grpRec["overrideGroupRecordSourceCount"].toBool());
                rec->set_source_count(grpRec["groupRecordSourceCount"].toUInt());
                // NOTE: rec->sources => subclass responsibility
                rec->set_is_override_aux_data_length(
                            grpRec["overrideAuxDataLength"].toBool());
                rec->set_aux_data_length(grpRec["auxDataLength"].toUInt());
                QByteArray ba = grpRec["auxData"].toByteArray();
                // pad to word boundary
                if (ba.size() % 4)
                    ba.append(QByteArray(4 - (ba.size() % 4), char(0)));
                rec->set_aux_data(std::string(ba.constData(), ba.size()));
            }

            break;
        }

        // Meta Fields
        case kIsOverrideChecksum:
        {
            bool ovr = value.toBool();
            data.set_is_override_checksum(ovr);
            isOk = true;
            break;
        }

        case kGroupMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.GroupMode_IsValid(mode))
                data.set_group_mode((OstProto::Gmp::GroupMode)mode);
            break;
        }
        case kGroupCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_group_count(count);
            break;
        }
        case kGroupPrefix:
        {
            uint prefix = value.toUInt(&isOk);
            if (isOk)
                data.set_group_prefix(prefix);
            break;
        }

        case kIsOverrideSourceCount:
        {
            bool ovr = value.toBool();
            data.set_is_override_source_count(ovr);
            isOk = true;
            break;
        }

        case kIsOverrideGroupRecordCount:
        {
            bool ovr = value.toBool();
            data.set_is_override_group_record_count(ovr);
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

int GmpProtocol::protocolFrameSize(int streamIndex) const
{
    // TODO: Calculate to reduce processing cost
    return AbstractProtocol::protocolFrameValue(streamIndex, true).size();
}

bool GmpProtocol::isProtocolFrameValueVariable() const
{
    // No fields vary for Ssm Report
    if (isSsmReport())
        return false;

    // For all other msg types, check the group mode
    if (fieldData(kGroupMode, FieldValue).toUInt() 
            != uint(OstProto::Gmp::kFixed))
        return true;

    return false;
}

int GmpProtocol::protocolFrameVariableCount() const
{
    int count = 1;

    // No fields vary for Ssm Report
    if (isSsmReport())
        return count;

    // For all other msg types, check the group mode
    if (fieldData(kGroupMode, FieldValue).toUInt() 
            != uint(OstProto::Gmp::kFixed))
    {
        count = AbstractProtocol::lcm(count,
                fieldData(kGroupCount, FieldValue).toUInt());
    }

    return count;
}
