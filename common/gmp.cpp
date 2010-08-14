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

#include <QHeaderView>
#include <qendian.h>

GmpConfigForm::GmpConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    msgTypeCombo->setValueMask(0xFF);
    msgTypeCombo->addItem(kIgmpV1Query,  "IGMPv1 Query");
    msgTypeCombo->addItem(kIgmpV1Report, "IGMPv1 Report");
    msgTypeCombo->addItem(kIgmpV2Query,  "IGMPv2 Query");
    msgTypeCombo->addItem(kIgmpV2Report, "IGMPv2 Report");
    msgTypeCombo->addItem(kIgmpV2Leave,  "IGMPv2 Leave");
    msgTypeCombo->addItem(kIgmpV3Query,  "IGMPv3 Query");
    msgTypeCombo->addItem(kIgmpV3Report, "IGMPv3 Report");

    auxData->setValidator(new QRegExpValidator(
                QRegExp("[0-9A-Fa-f]*"), this));
}

GmpConfigForm::~GmpConfigForm()
{
    // delete UserRole itemdata for grpRecords
    for (int i = 0; i < groupList->count(); i++)
    {
        QListWidgetItem *item = groupList->item(i);

        if (item)
        {
            OstProto::Gmp::GroupRecord *rec = (OstProto::Gmp::GroupRecord*)
                item->data(Qt::UserRole).value<void*>();
            item->setData(Qt::UserRole, QVariant());
            delete rec;
        }
    }
}

void GmpConfigForm::on_msgTypeCombo_currentIndexChanged(int /*index*/)
{
    switch(msgTypeCombo->currentValue())
    {
    case kIgmpV1Query:
    case kIgmpV1Report:
    case kIgmpV2Query:
    case kIgmpV2Report:
    case kIgmpV2Leave:
    case kMldV1Query:
    case kMldV1Report:
    case kMldV1Done:
        asmGroup->show();
        ssmWidget->hide();
        break;

    case kIgmpV3Query:
    case kMldV2Query:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(0);
        ssmWidget->show();
        break;

    case kIgmpV3Report:
    case kMldV2Report:
        asmGroup->hide();
        ssmWidget->setCurrentIndex(1);
        ssmWidget->show();
        break;

    default:
        asmGroup->hide();
        ssmWidget->hide();
        break;
    }
}

void GmpConfigForm::on_addSource_clicked()
{
    QListWidgetItem *item=new QListWidgetItem(_defaultSourceIp);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    sourceList->insertItem(sourceList->currentRow(), item);

    if (!overrideSourceCount->isChecked())
        sourceCount->setText(QString().setNum(sourceList->count()));
}

void GmpConfigForm::on_deleteSource_clicked()
{
    delete sourceList->takeItem(sourceList->currentRow());

    if (!overrideSourceCount->isChecked())
        sourceCount->setText(QString().setNum(sourceList->count()));
}

void GmpConfigForm::on_addGroupRecord_clicked()
{
    OstProto::Gmp::GroupRecord *record = new OstProto::Gmp::GroupRecord;
    QListWidgetItem *item = new QListWidgetItem;

    item->setData(Qt::UserRole, QVariant::fromValue((void*)record));
    item->setText("xxx"); // FIXME

    groupList->insertItem(groupList->currentRow(), item);

    if (!overrideGroupRecordCount->isChecked())
        groupRecordCount->setText(QString().setNum(groupList->count()));
}

void GmpConfigForm::on_deleteGroupRecord_clicked()
{
    QListWidgetItem *item = groupList->takeItem(groupList->currentRow());
    if (item)
    {
        delete (OstProto::Gmp::GroupRecord*)item->data(Qt::UserRole)
            .value<void*>();
        delete item;
    }

    if (!overrideGroupRecordCount->isChecked())
        groupRecordCount->setText(QString().setNum(groupList->count()));
}

void GmpConfigForm::on_groupList_currentItemChanged(QListWidgetItem *current,
        QListWidgetItem *previous)
{
    OstProto::Gmp::GroupRecord *prevRec;
    OstProto::Gmp::GroupRecord *currRec;
    
    qDebug("in %s", __FUNCTION__);

    // save previous record ...
    if (previous == NULL)
        goto _load_current_record;

    prevRec = (OstProto::Gmp::GroupRecord*)previous->data(Qt::UserRole)
        .value<void*>();

    prevRec->set_type(OstProto::Gmp::GroupRecord::RecordType(
                groupRecordType->currentIndex()+1));
    // FIXME: groupRecordAddress, sources

    prevRec->set_is_override_source_count(
            overrideGroupRecordSourceCount->isChecked());
    prevRec->set_source_count(groupRecordSourceCount->text().toUInt());

    prevRec->set_is_override_aux_data_length(
            overrideAuxDataLength->isChecked());
    prevRec->set_aux_data(QString(QByteArray::fromHex(
            QByteArray().append(auxData->text()))).toStdString());

_load_current_record:
    // ... and load current record
    if (current == NULL)
        goto _exit;

    currRec = (OstProto::Gmp::GroupRecord*)current->data(Qt::UserRole)
        .value<void*>();

    groupRecordType->setCurrentIndex(int(currRec->type()) - 1);
    // FIXME: groupRecordAddress, sources

    overrideGroupRecordSourceCount->setChecked(
            currRec->is_override_source_count());
    if (overrideGroupRecordSourceCount->isChecked())
    {
        groupRecordSourceCount->setText(
                QString().setNum(currRec->source_count()));
    }

    overrideAuxDataLength->setChecked(currRec->is_override_aux_data_length());
    if (overrideAuxDataLength->isChecked())
        auxDataLength->setText(QString().setNum(currRec->aux_data_length()));
    auxData->setText(QString(QByteArray().append(
                QString().fromStdString(currRec->aux_data())).toHex()));
_exit:
    groupRecord->setEnabled(current != NULL);
    return;
}

void GmpConfigForm::on_addGroupRecordSource_clicked()
{
    QListWidgetItem *item=new QListWidgetItem(_defaultSourceIp);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    groupRecordSourceList->insertItem(groupRecordSourceList->currentRow(),item);

    if (!overrideGroupRecordSourceCount->isChecked())
        groupRecordSourceCount->setText(QString().setNum(
                    groupRecordSourceList->count()));
}

void GmpConfigForm::on_deleteGroupRecordSource_clicked()
{
    delete groupRecordSourceList->takeItem(groupRecordSourceList->currentRow());

    if (!overrideGroupRecordSourceCount->isChecked())
        groupRecordSourceCount->setText(QString().setNum(
                    groupRecordSourceList->count()));
}

void GmpConfigForm::on_auxData_textChanged(const QString &text)
{
    if (!overrideAuxDataLength->isChecked())
        auxDataLength->setText(QString().setNum(auxData->text().length()/2));
}

GmpProtocol::GmpProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    /* The configWidget is created lazily */
    configForm = NULL;
}

GmpProtocol::~GmpProtocol()
{
    delete configForm;
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
    int count = 0;

    // TODO: optimize!!!!!
    for (int i = 0; i < FIELD_COUNT; i++)
    {
        if (fieldFlags(i).testFlag(AbstractProtocol::FrameField))
            count++;
    }
    return count;
#if 0
    switch(msgType())
    {
        // IGMP
    	case kIgmpV1Query:
    	case kIgmpV1Report:
    	case kIgmpV2Query:
    	case kIgmpV2Report:
    	case kIgmpV2Leave:
            return FIELD_COUNT_ASM_ALL;

    	case kIgmpV3Query:
            return FIELD_COUNT_SSM_QUERY;
    	case kIgmpV3Report:
            return FIELD_COUNT_SSM_REPORT;

        // MLD
    	case kMldV1Query:
    	case kMldV1Report:
    	case kMldV1Done:
            return FIELD_COUNT_ASM_ALL;

    	case kMldV2Query:
            return FIELD_COUNT_SSM_QUERY;
    	case kMldV2Report:
            return FIELD_COUNT_SSM_REPORT;

        default:
            return FIELD_COUNT_ASM_ALL;
    }
#endif
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
            quint16 cksum = data.is_override_checksum() ?  
                data.checksum() : checksum(streamIndex); 

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
                return QString("0x%1").arg(cksum, 4, BASE_HEX, QChar('0'));
            case FieldBitSize:
                return 16;
            default:
                break;
            }
            break;
        }
        case kMldMrt:
            // XXX: Present only in MLD - hence handled by the mld subclass
            break;

        case kGroupAddress:
            // XXX: Handled by each subclass
            break;

        case kRsvd1:
        {
            int rsvd = 0;

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
                    quint8 qqic = qqi; // TODO: derive code from qqi
                    return QByteArray(1, char(qqic));
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
            // XXX:Handled by each subclass
            break;

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
            uint qqi = value.toUInt(&isOk); // TODO: QQIC or QQI??
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
            // XXX: Handled by subclass
            break;

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

/*!
  TODO: Return the protocol frame size in bytes\n

  If your protocol has a fixed size - you don't need to reimplement this; the
  base class implementation is good enough
*/
int GmpProtocol::protocolFrameSize(int streamIndex) const
{
    // TODO: Calculate to reduce processing cost
    return AbstractProtocol::protocolFrameValue(streamIndex, true).size();
}

/*!
  TODO: If your protocol has any variable fields, return true \n

  Otherwise you don't need to reimplement this method - the base class always
  returns false
*/
bool GmpProtocol::isProtocolFrameValueVariable() const
{
    // TODO: check msg types and then return value appropriately
    return true;
}

void GmpProtocol::loadConfigWidget()
{
    configWidget();

    configForm->msgTypeCombo->setValue(fieldData(kType, FieldValue).toUInt());
    configForm->maxResponseTime->setText(QString("%1").arg(
                data.max_response_time()));
    configForm->overrideChecksum->setChecked(
            fieldData(kIsOverrideChecksum, FieldValue).toBool());
    configForm->checksum->setText(uintToHexStr(
            fieldData(kChecksum, FieldValue).toUInt(), 2));

    configForm->groupAddress->setText(
            fieldData(kGroupAddress, FieldValue).toString());
    configForm->groupMode->setCurrentIndex(
            fieldData(kGroupMode, FieldValue).toUInt());
    configForm->groupCount->setText(
            fieldData(kGroupCount, FieldValue).toString());
    configForm->groupPrefix->setText(
            fieldData(kGroupPrefix, FieldValue).toString());

    configForm->sFlag->setChecked(fieldData(kSFlag, FieldValue).toBool());
    configForm->qrv->setText(fieldData(kQrv, FieldValue).toString());
    configForm->qqi->setText(fieldData(kQqic, FieldValue).toString());

    configForm->sourceList->clear();
    configForm->sourceList->addItems(
            fieldData(kSources, FieldValue).toStringList());

    configForm->overrideSourceCount->setChecked(
            fieldData(kIsOverrideSourceCount, FieldValue).toBool());
    configForm->sourceCount->setText(
            fieldData(kSourceCount, FieldValue).toString());

    configForm->overrideGroupRecordCount->setChecked(
            fieldData(kIsOverrideGroupRecordCount, FieldValue).toBool());
    configForm->groupRecordCount->setText(
            fieldData(kGroupRecordCount, FieldValue).toString());
    // TODO: groups
}

void GmpProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    setFieldData(kType, configForm->msgTypeCombo->currentValue());
    setFieldData(kMldMrt, configForm->maxResponseTime->text());
    setFieldData(kIsOverrideChecksum, 
            configForm->overrideChecksum->isChecked());
    setFieldData(kChecksum, 
            configForm->checksum->text().toUInt(&isOk, BASE_HEX));

    setFieldData(kGroupAddress, configForm->groupAddress->text());
    setFieldData(kGroupMode, configForm->groupMode->currentIndex());
    setFieldData(kGroupCount, configForm->groupCount->text());
    setFieldData(kGroupPrefix, configForm->groupPrefix->text().remove('/'));

    setFieldData(kSFlag, configForm->sFlag->isChecked());
    setFieldData(kQrv, configForm->qrv->text());
    setFieldData(kQqic, configForm->qqi->text());

    QStringList list;
    for (int i = 0; i < configForm->sourceList->count(); i++)
        list.append(configForm->sourceList->item(i)->text());
    setFieldData(kSources, list);

    // XXX: sourceCount should be AFTER sources
    setFieldData(kIsOverrideSourceCount, 
            configForm->overrideSourceCount->isChecked());
    setFieldData(kSourceCount, configForm->sourceCount->text());

    // TODO: Group Records

    // XXX: groupRecordCount should be AFTER groupRecords
    setFieldData(kIsOverrideGroupRecordCount, 
            configForm->overrideGroupRecordCount->isChecked());
    setFieldData(kGroupRecordCount, configForm->groupRecordCount->text());
#if 0
    setFieldData(kA, configForm->gmpA->text());
    setFieldData(kB, configForm->gmpB->text());

    setFieldData(kPayloadLength, configForm->gmpPayloadLength->text());
    setFieldData(kIs_override_checksum, 
        configForm->isChecksumOverride->isChecked());
    setFieldData(kChecksum, configForm->gmpChecksum->text().toUInt(&isOk, BASE_HEX));

    setFieldData(kX, configForm->gmpX->text());
    setFieldData(kY, configForm->gmpY->text());
#endif
}
