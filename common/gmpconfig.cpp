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

#include "gmpconfig.h"
#include "gmp.h"

GmpConfigForm::GmpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    auxData->setValidator(new QRegExpValidator(
                QRegExp("[0-9A-Fa-f]*"), this));
}

GmpConfigForm::~GmpConfigForm()
{
}

void GmpConfigForm::loadWidget(AbstractProtocol *proto)
{
    msgTypeCombo->setValue(
            proto->fieldData(
                GmpProtocol::kType,
                AbstractProtocol::FieldValue
            ).toUInt());

    // XXX: maxResponseTime set by subclass
    
    overrideChecksum->setChecked(
            proto->fieldData(
                GmpProtocol::kIsOverrideChecksum,
                AbstractProtocol::FieldValue
            ).toBool());
    checksum->setText(uintToHexStr(
            proto->fieldData(
                GmpProtocol::kChecksum,
                AbstractProtocol::FieldValue
            ).toUInt(), 2));

    groupAddress->setText(
            proto->fieldData(
                GmpProtocol::kGroupAddress,
                AbstractProtocol::FieldValue
            ).toString());
    groupMode->setCurrentIndex(
            proto->fieldData(
                GmpProtocol::kGroupMode,
                AbstractProtocol::FieldValue
            ).toUInt());
    groupCount->setText(
            proto->fieldData(
                GmpProtocol::kGroupCount,
                AbstractProtocol::FieldValue
            ).toString());
    groupPrefix->setText(
            proto->fieldData(
                GmpProtocol::kGroupPrefix,
                AbstractProtocol::FieldValue
            ).toString());

    sFlag->setChecked(
            proto->fieldData(
                GmpProtocol::kSFlag,
                AbstractProtocol::FieldValue
            ).toBool());
    qrv->setText(
            proto->fieldData(
                GmpProtocol::kQrv,
                AbstractProtocol::FieldValue
            ).toString());
    qqi->setText(
            proto->fieldData(
                GmpProtocol::kQqic,
                AbstractProtocol::FieldValue
            ).toString());

    QStringList sl = 
            proto->fieldData(
                GmpProtocol::kSources,
                AbstractProtocol::FieldValue
            ).toStringList();
    sourceList->clear();
    foreach(QString src, sl)
    {
        QListWidgetItem *item = new QListWidgetItem(src);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        sourceList->addItem(item);
    }

    // NOTE: SourceCount should be loaded after sourceList
    overrideSourceCount->setChecked(
            proto->fieldData(
                GmpProtocol::kIsOverrideSourceCount,
                AbstractProtocol::FieldValue
            ).toBool());
    sourceCount->setText(
            proto->fieldData(
                GmpProtocol::kSourceCount,
                AbstractProtocol::FieldValue
            ).toString());

    QVariantList list = 
            proto->fieldData(
                GmpProtocol::kGroupRecords,
                AbstractProtocol::FieldValue
            ).toList();
    groupList->clear();
    foreach (QVariant rec, list)
    {
        QVariantMap grpRec = rec.toMap();
        QListWidgetItem *item = new QListWidgetItem;

        item->setData(Qt::UserRole, grpRec);
        item->setText(QString("%1: %2")
                .arg(groupRecordType->itemText(
                        grpRec["groupRecordType"].toInt()))
                .arg(grpRec["groupRecordAddress"].toString()));
        groupList->addItem(item);
    }

    // NOTE: recordCount should be loaded after recordList
    overrideGroupRecordCount->setChecked(
            proto->fieldData(
                GmpProtocol::kIsOverrideGroupRecordCount,
                AbstractProtocol::FieldValue
            ).toBool());
    groupRecordCount->setText(
            proto->fieldData(
                GmpProtocol::kGroupRecordCount,
                AbstractProtocol::FieldValue
            ).toString());
}

void GmpConfigForm::storeWidget(AbstractProtocol *proto)
{
    update();

    proto->setFieldData(
            GmpProtocol::kType, 
            msgTypeCombo->currentValue());

    // XXX: maxResponseTime handled by subclass

    proto->setFieldData(
            GmpProtocol::kIsOverrideChecksum, 
            overrideChecksum->isChecked());
    proto->setFieldData(
            GmpProtocol::kChecksum, 
            hexStrToUInt(checksum->text()));

    proto->setFieldData(
            GmpProtocol::kGroupAddress, 
            groupAddress->text());
    proto->setFieldData(
            GmpProtocol::kGroupMode, 
            groupMode->currentIndex());
    proto->setFieldData(
            GmpProtocol::kGroupCount, 
            groupCount->text());
    proto->setFieldData(
            GmpProtocol::kGroupPrefix, 
            groupPrefix->text().remove('/'));

    proto->setFieldData(
            GmpProtocol::kSFlag, 
            sFlag->isChecked());
    proto->setFieldData(
            GmpProtocol::kQrv, 
            qrv->text());
    proto->setFieldData(
            GmpProtocol::kQqic, 
            qqi->text());

    QStringList list;
    for (int i = 0; i < sourceList->count(); i++)
        list.append(sourceList->item(i)->text());

    proto->setFieldData(
            GmpProtocol::kSources,
            list);

    // sourceCount should be AFTER sources
    proto->setFieldData(
            GmpProtocol::kIsOverrideSourceCount, 
            overrideSourceCount->isChecked());
    proto->setFieldData(
            GmpProtocol::kSourceCount, 
            sourceCount->text());

    QVariantList grpList;
    for (int i = 0; i < groupList->count(); i++)
    {
        QVariant grp = groupList->item(i)->data(Qt::UserRole);
        grpList.append(grp.toMap());
    }
    proto->setFieldData(GmpProtocol::kGroupRecords, grpList);

    // groupRecordCount should be AFTER groupRecords
    proto->setFieldData(
            GmpProtocol::kIsOverrideGroupRecordCount, 
            overrideGroupRecordCount->isChecked());
    proto->setFieldData(
            GmpProtocol::kGroupRecordCount, 
            groupRecordCount->text());
}

void GmpConfigForm::update()
{
    // save the current group Record by simulating a currentItemChanged()
    on_groupList_currentItemChanged(groupList->currentItem(), 
            groupList->currentItem());
}

//
// -- private slots
//

void GmpConfigForm::on_groupMode_currentIndexChanged(int index)
{
    bool disabled = (index == 0);

    groupCount->setDisabled(disabled);
    groupPrefix->setDisabled(disabled);
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
    OstProto::Gmp::GroupRecord defRec;
    QVariantMap grpRec;
    QListWidgetItem *item = new QListWidgetItem;

    grpRec["groupRecordType"] = defRec.type();
    grpRec["groupRecordAddress"] = _defaultGroupIp;
    grpRec["overrideGroupRecordSourceCount"] =defRec.is_override_source_count();
    grpRec["groupRecordSourceCount"] = defRec.source_count();
    grpRec["groupRecordSourceList"] = QStringList();
    grpRec["overrideAuxDataLength"] = defRec.is_override_aux_data_length();
    grpRec["auxDataLength"] = defRec.aux_data_length();
    grpRec["auxData"] = QByteArray().append(
            QString().fromStdString(defRec.aux_data()));

    item->setData(Qt::UserRole, grpRec);
    item->setText(QString("%1: %2")
            .arg(groupRecordType->itemText(grpRec["groupRecordType"].toInt()))
            .arg(grpRec["groupRecordAddress"].toString()));

    groupList->insertItem(groupList->currentRow(), item);

    if (!overrideGroupRecordCount->isChecked())
        groupRecordCount->setText(QString().setNum(groupList->count()));
}

void GmpConfigForm::on_deleteGroupRecord_clicked()
{
    delete groupList->takeItem(groupList->currentRow());

    if (!overrideGroupRecordCount->isChecked())
        groupRecordCount->setText(QString().setNum(groupList->count()));
}

void GmpConfigForm::on_groupList_currentItemChanged(QListWidgetItem *current,
        QListWidgetItem *previous)
{
    QVariantMap rec;
    QStringList strList;
    
    qDebug("in %s", __FUNCTION__);

    // save previous record ...
    if (previous == NULL)
        goto _load_current_record;

    rec["groupRecordType"] = groupRecordType->currentIndex();
    rec["groupRecordAddress"] = groupRecordAddress->text();
    strList.clear();
    while (groupRecordSourceList->count())
    {
        QListWidgetItem *item = groupRecordSourceList->takeItem(0);
        strList.append(item->text());
        delete item;
    }
    rec["groupRecordSourceList"] = strList;
    rec["overrideGroupRecordSourceCount"] = 
                            overrideGroupRecordSourceCount->isChecked();
    rec["groupRecordSourceCount"] = groupRecordSourceCount->text().toUInt();
    rec["overrideAuxDataLength"] = overrideAuxDataLength->isChecked();
    rec["auxDataLength"] = auxDataLength->text().toUInt();
    rec["auxData"] = QByteArray().fromHex(QByteArray().append(auxData->text()));

    previous->setData(Qt::UserRole, rec);
    previous->setText(QString("%1: %2")
            .arg(groupRecordType->itemText(rec["groupRecordType"].toInt()))
            .arg(rec["groupRecordAddress"].toString()));

_load_current_record:
    // ... and load current record
    if (current == NULL)
        goto _exit;

    rec = current->data(Qt::UserRole).toMap();

    groupRecordType->setCurrentIndex(rec["groupRecordType"].toInt());
    groupRecordAddress->setText(rec["groupRecordAddress"].toString());
    strList = rec["groupRecordSourceList"].toStringList();
    groupRecordSourceList->clear();
    foreach (QString str, strList)
    {
        QListWidgetItem *item = new QListWidgetItem(str, groupRecordSourceList);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    overrideGroupRecordSourceCount->setChecked(
                        rec["overrideGroupRecordSourceCount"].toBool());
    groupRecordSourceCount->setText(QString().setNum(
                        rec["groupRecordSourceCount"].toUInt()));
    overrideAuxDataLength->setChecked(rec["overrideAuxDataLength"].toBool());
    auxDataLength->setText(QString().setNum(rec["auxDataLength"].toUInt()));
    auxData->setText(QString(rec["auxData"].toByteArray().toHex()));

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
    // auxDataLength is in units of words and each byte is 2 chars in text()
    if (!overrideAuxDataLength->isChecked())
        auxDataLength->setText(QString().setNum((text.size()+7)/8));
}
