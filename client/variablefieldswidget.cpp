/*
Copyright (C) 2015 Srivats P.

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

#include "variablefieldswidget.h"

#include "abstractprotocol.h"
#include "protocollistiterator.h"
#include "stream.h"

#include <QListWidgetItem>
#include <QMetaType>
#include <QStringList>

Q_DECLARE_METATYPE(AbstractProtocol*);
Q_DECLARE_METATYPE(OstProto::VariableField);

QStringList typeNames = QStringList()
    << "Counter8"
    << "Counter16"
    << "Counter32";

QStringList modeNames = QStringList()
    << "Increment"
    << "Decrement"
    << "Random";

#define uintToHexStr(num, bytes)    \
    QString("%1").arg(num, bytes*2, BASE_HEX, QChar('0')).toUpper()
#define hexStrToUInt(str)    \
    str.toUInt(NULL, BASE_HEX)

/* 
 * NOTES:
 * 1. We use a QSpinBox for all numeric values except for 'value' because 
 *    QSpinBox value is of type 'int' - we would like the ability to store
 *    quint32
 * 2. This widget will keep the stream always updated - every editing change
 *    of a attribute is immediately updated in the stream; the consequence
 *    of this design is that an explicit 'store' of widget contents to the
 *    stream is no longer required - we still define a store() method in
 *    case we need to change the design later
 */

VariableFieldsWidget::VariableFieldsWidget(QWidget *parent)
    : QWidget(parent)
{
    stream_ = NULL;
    isProgLoad_ = false;
    lastSelectedProtocolIndex_ = 0;

    setupUi(this);
    attribGroup->setHidden(true);

    type->addItems(typeNames);
    mode->addItems(modeNames);

    valueRange_ = new QIntValidator(this);
    // FIXME: we can't use QIntValidator - since we want value to be able
    // to enter a quint32
    //value->setValidator(valueRange_);

    connect(type, SIGNAL(currentIndexChanged(int)), 
            SLOT(updateCurrentVariableField()));
    connect(offset, SIGNAL(valueChanged(int)), 
            SLOT(updateCurrentVariableField()));
    connect(bitmask, SIGNAL(textChanged(QString)), 
            SLOT(updateCurrentVariableField()));

    connect(mode, SIGNAL(currentIndexChanged(int)), 
            SLOT(updateCurrentVariableField()));
    connect(value, SIGNAL(textChanged(QString)), 
            SLOT(updateCurrentVariableField()));
    connect(count, SIGNAL(valueChanged(int)), 
            SLOT(updateCurrentVariableField()));
    connect(step, SIGNAL(valueChanged(int)), 
            SLOT(updateCurrentVariableField()));
}

void VariableFieldsWidget::setStream(Stream *stream)
{
    stream_ = stream;
}

void VariableFieldsWidget::load()
{
    Q_ASSERT(stream_);
    Q_ASSERT(protocolList->count() == 0);
    Q_ASSERT(variableFieldList->count() == 0);

    ProtocolListIterator *iter = stream_->createProtocolListIterator();
    while (iter->hasNext()) {
        AbstractProtocol *proto = iter->next();
        QListWidgetItem *protoItem = new QListWidgetItem;

        protoItem->setData(kProtocolPtrRole, QVariant::fromValue(proto));
        protoItem->setData(kCurrentVarFieldRole,
                QVariant::fromValue(proto->variableFieldCount() ? 0 : -1));
        protoItem->setText(proto->shortName());
        decorateProtocolItem(protoItem);

        protocolList->addItem(protoItem);
    }
    delete iter;

    if (lastSelectedProtocolIndex_  < protocolList->count())
        protocolList->setCurrentRow(lastSelectedProtocolIndex_);

    // XXX: protocolList->setCurrentRow() above will emit currentItemChanged
    // which will load variableFieldsList - no need to load it explicitly
}

void VariableFieldsWidget::store()
{
    /* Do Nothing - see Notes at the top of the file */
}

void VariableFieldsWidget::clear()
{
    protocolList->clear();
    variableFieldList->clear();
}

void VariableFieldsWidget::on_protocolList_currentItemChanged(
        QListWidgetItem *current,
        QListWidgetItem *previous)
{
    AbstractProtocol *proto;

    qDebug("%s: curr = %p, prev = %p", __FUNCTION__, current, previous);

    if (current == NULL)
        goto _exit;

    proto = current->data(kProtocolPtrRole).value<AbstractProtocol*>();
    loadProtocolFields(proto);

    variableFieldList->clear();
    for (int i = 0; i < proto->variableFieldCount(); i++) {
        OstProto::VariableField vf = proto->variableField(i);
        QListWidgetItem *vfItem = new QListWidgetItem;

        setVariableFieldItem(vfItem, proto, vf);
        variableFieldList->addItem(vfItem);
    }

    // While switching protocols, we want to setup the attrib group 
    // validation/ranges/masks for the current protocol, which is done 
    // by the field/type signal handlers - so clear field/type index
    // now so that signals are emitted when we add/select a VF
    field->setCurrentIndex(-1);
    type->setCurrentIndex(-1);

    variableFieldList->setCurrentRow(
            current->data(kCurrentVarFieldRole).value<int>());

    lastSelectedProtocolIndex_ = protocolList->currentRow();

_exit:
    addButton->setEnabled(current != NULL);
}

void VariableFieldsWidget::on_variableFieldList_currentItemChanged(
        QListWidgetItem *current,
        QListWidgetItem *previous)
{
    OstProto::VariableField vf;

    qDebug("%s: curr = %p, prev = %p", 
            __FUNCTION__, current, previous);

    if (current == NULL)
        goto _exit;

    vf = current->data(kVarFieldRole).value<OstProto::VariableField>();

    isProgLoad_ = true;
    
    field->setCurrentIndex(fieldIndex(vf));
    type->setCurrentIndex(vf.type());
    offset->setValue(vf.offset());
    bitmask->setText(uintToHexStr(vf.mask(), typeSize(vf.type())));
    value->setText(QString().setNum(vf.value()));
    mode->setCurrentIndex(vf.mode());
    count->setValue(vf.count());
    step->setValue(vf.step());

    isProgLoad_ = false;

    protocolList->currentItem()->setData(
            kCurrentVarFieldRole,
            QVariant::fromValue(variableFieldList->currentRow()));
_exit:
    attribGroup->setHidden(current == NULL);
    deleteButton->setEnabled(current != NULL);
}

void VariableFieldsWidget::on_addButton_clicked()
{
    QListWidgetItem *protoItem = protocolList->currentItem();

    if (!protoItem)
        return;

    AbstractProtocol *proto = protoItem->data(kProtocolPtrRole)
                                            .value<AbstractProtocol*>();
    OstProto::VariableField vf;
    QListWidgetItem *vfItem = new QListWidgetItem;

    proto->appendVariableField(vf);
    setVariableFieldItem(vfItem, proto, vf);
    variableFieldList->addItem(vfItem);
    variableFieldList->setCurrentItem(vfItem);

    decorateProtocolItem(protoItem);
}

void VariableFieldsWidget::on_deleteButton_clicked()
{
    QListWidgetItem *protoItem = protocolList->currentItem();
    int vfIdx = variableFieldList->currentRow();

    if (!protoItem || (vfIdx < 0))
        return;

    AbstractProtocol *proto = protoItem->data(kProtocolPtrRole)
                                            .value<AbstractProtocol*>();
    proto->removeVariableField(vfIdx);
    delete variableFieldList->takeItem(vfIdx);

    // XXX: takeItem() above triggers a currentChanged, but the signal
    // is emitted after the "current" is changed to an item after
    // or before the item(s) to be deleted but before the item(s)
    // are actually deleted - so the current inside that slot is not
    // correct and we need to re-save it again
    protocolList->currentItem()->setData(
            kCurrentVarFieldRole,
            QVariant::fromValue(variableFieldList->currentRow()));

    decorateProtocolItem(protoItem);
}

void VariableFieldsWidget::on_field_currentIndexChanged(int index)
{
    if (index < 0)
        return;

    QVariantMap vm = field->itemData(index).toMap();

    if (index) { // standard frame fields
        offset->setValue(vm["offset"].toUInt());
        offset->setDisabled(true);
        type->setCurrentIndex(vm["type"].toUInt());
        type->setDisabled(true);
        bitmask->setText(uintToHexStr(
                            vm["mask"].toUInt(), 
                            typeSize(OstProto::VariableField::Type(
                                        vm["type"].toUInt()))));
        bitmask->setDisabled(true);
    }
    else { // custom field
        offset->setEnabled(true);
        type->setEnabled(true);
        bitmask->setEnabled(true);
    }
}

void VariableFieldsWidget::on_type_currentIndexChanged(int index)
{
    if ((index < 0) || !protocolList->currentItem())
        return;

    AbstractProtocol *proto = protocolList->currentItem()
                                    ->data(kProtocolPtrRole)
                                        .value<AbstractProtocol*>();
    int protoSize = proto->protocolFrameSize();

    switch (index)
    {
    case OstProto::VariableField::kCounter8:
        offset->setRange(0, protoSize - 1);
        bitmask->setInputMask("HH");
        bitmask->setText("FF");
        valueRange_->setRange(0, 0xFF);
        count->setRange(0, 0xFF);
        step->setRange(0, 0xFF);
        break;
    case OstProto::VariableField::kCounter16:
        offset->setRange(0, protoSize - 2);
        bitmask->setInputMask("HHHH");
        bitmask->setText("FFFF");
        valueRange_->setRange(0, 0xFFFF);
        count->setRange(0, 0xFFFF);
        step->setRange(0, 0xFFFF);
        break;
    case OstProto::VariableField::kCounter32:
        offset->setRange(0, protoSize - 4);
        bitmask->setInputMask("HHHHHHHH");
        bitmask->setText("FFFFFFFF");
        valueRange_->setRange(0, 0xFFFFFFFF);
        count->setRange(0, 0x7FFFFFFF);
        step->setRange(0, 0x7FFFFFFF);
        break;
    default:
        Q_ASSERT(false); // unreachable
        break;
    }
}

void VariableFieldsWidget::updateCurrentVariableField()
{
    // Prevent recursion
    if (isProgLoad_)
        return;

    if (!protocolList->currentItem())
        return;
    if (!variableFieldList->currentItem())
        return;

    OstProto::VariableField vf;

    vf.set_type(OstProto::VariableField::Type(type->currentIndex()));
    vf.set_offset(offset->value());
    vf.set_mask(hexStrToUInt(bitmask->text()));
    vf.set_value(value->text().toUInt());
    vf.set_mode(OstProto::VariableField::Mode(mode->currentIndex()));
    vf.set_count(count->value());
    vf.set_step(step->value());

    QListWidgetItem *protoItem = protocolList->currentItem();
    AbstractProtocol *proto = protoItem->data(kProtocolPtrRole)
                                            .value<AbstractProtocol*>();
    proto->mutableVariableField(variableFieldList->currentRow())->CopyFrom(vf);
    setVariableFieldItem(variableFieldList->currentItem(), proto, vf);
}

void VariableFieldsWidget::decorateProtocolItem(QListWidgetItem *item)
{
    AbstractProtocol *proto = item->data(kProtocolPtrRole)
                                            .value<AbstractProtocol*>();
    QFont font = item->font();
    font.setBold(proto->variableFieldCount() > 0);
    item->setFont(font);
}

void VariableFieldsWidget::loadProtocolFields(
        const AbstractProtocol *protocol)
{
    QVariantMap vm;

    field->clear();

    field->addItem("Custom");
    for (int i = 0; i < protocol->fieldCount(); i++) {
        if (!protocol->fieldFlags(i).testFlag(AbstractProtocol::FrameField))
            continue;
        QString name = protocol->fieldData(i, AbstractProtocol::FieldName)
                                        .toString();
        int bitOfs = protocol->fieldFrameBitOffset(i);
        int byteOfs = bitOfs >> 3;
        uint bitSize = protocol->fieldData(i, AbstractProtocol::FieldBitSize)
                                        .toInt();
        vm["offset"] = byteOfs;
        if (bitSize <= 8) {
            vm["type"] = int(OstProto::VariableField::kCounter8);
            vm["mask"] = ((0xFF << (8 - bitSize)) & 0xFF) 
                                >> (bitOfs & 0x7);
        }
        else if (bitSize <= 16) {
            vm["type"] = int(OstProto::VariableField::kCounter16);
            vm["mask"] = ((0xFFFF << (16 - bitSize)) & 0xFFFF)
                                >> (bitOfs & 0x7);
        }
        else if (bitSize <= 32) {
            vm["type"] = int(OstProto::VariableField::kCounter32);
            vm["mask"] = ((0xFFFFFFFF << (32 - bitSize)) & 0xFFFFFFFF)
                                >> (bitOfs & 0x7);
        }
        else { 
            vm["type"] = int(OstProto::VariableField::kCounter32);
            vm["mask"] = 0xFFFFFFFF;
        }
        field->addItem(name, vm);
    }
}

/*! Given a VariableField::Type, return corresponding size in bytes */
int VariableFieldsWidget::typeSize(OstProto::VariableField::Type type)
{
    switch(type) {
        case OstProto::VariableField::kCounter8 : return 1;
        case OstProto::VariableField::kCounter16: return 2;
        case OstProto::VariableField::kCounter32: return 4;
        default: break;
    }

    return 4;
}

/*! Given a variableField, return corresponding index in the field ComboBox */
int VariableFieldsWidget::fieldIndex(const OstProto::VariableField &vf)
{
    QVariantMap vm;

    vm["type"] = int(vf.type());
    vm["offset"] = vf.offset();
    vm["mask"] = vf.mask();

    int index = field->findData(vm);
    qDebug("vm %d %d 0x%x => index %d", vf.type(), vf.offset(), vf.mask(), index);
    // Not found? Use 'Custom'
    if (index < 0)
        index = 0;

    return index;
}

void VariableFieldsWidget::setVariableFieldItem(
        QListWidgetItem *item,
        const AbstractProtocol *protocol,
        const OstProto::VariableField &vf)
{
    uint from = vf.value() & vf.mask();
    uint to;
    QString fieldName = field->itemText(fieldIndex(vf));
    QString itemText;

    if (vf.mode() == OstProto::VariableField::kDecrement)
        to = (vf.value() - (vf.count()-1)*vf.step()) & vf.mask();
    else
        to = (vf.value() + (vf.count()-1)*vf.step()) & vf.mask();

    item->setData(kVarFieldRole, QVariant::fromValue(vf));
    itemText = QString("%1 %2 %3 from %4 to %5")
            .arg(protocol->shortName())
            .arg(fieldName)
            .arg(modeNames.at(vf.mode()))
            .arg(from)
            .arg(to);
    if (vf.step() != 1)
        itemText.append(QString(" step %1").arg(vf.step()));
    item->setText(itemText);
}

