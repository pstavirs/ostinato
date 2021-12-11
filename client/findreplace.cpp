/*
Copyright (C) 2021 Srivats P.

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

#include "findreplace.h"

#include "abstractprotocol.h"
#include "iputils.h"
#include "protocolmanager.h"
#include "stream.h"
#include "uint128.h"

#include <QPushButton>

extern ProtocolManager *OstProtocolManager;

FindReplaceDialog::FindReplaceDialog(Action *action, QWidget *parent)
    : QDialog(parent), action_(action)
{
    setupUi(this);

    findMask->setMaskMode(true);
    replaceMask->setMaskMode(true);

    // Keep things simple and don't use mask(s) (default)
    useFindMask->setChecked(false);
    useReplaceMask->setChecked(false);

    // TODO: remove combo protocols - see note in StreamBase::findReplace
    QStringList protocolList = OstProtocolManager->protocolDatabase();
    protocolList.sort();
    protocol->addItems(protocolList);

    // Enable this setting if we have streams selected on input
    selectedStreamsOnly->setEnabled(action->selectedStreamsOnly);

    // Reset for user input
    action->selectedStreamsOnly = false;

    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Replace All"));
}

void FindReplaceDialog::on_protocol_currentIndexChanged(const QString &name)
{
    field->clear();
    fieldAttrib_.clear();

    Stream stream;
    AbstractProtocol *protocol = OstProtocolManager->createProtocol(
                                                            name, &stream);
    int count = protocol->fieldCount();
    for (int i = 0; i < count; i++) {
        // XXX: It might be useful to support meta fields too, later!
        if (!protocol->fieldFlags(i).testFlag(AbstractProtocol::FrameField))
            continue;

        int bitSize = protocol->fieldData(i, AbstractProtocol::FieldBitSize)
                                        .toInt();
        if (bitSize <= 0) // skip optional fields
            continue;

        FieldAttrib fieldAttrib;
        fieldAttrib.index = i; // fieldIndex
        fieldAttrib.bitSize = bitSize;

        // field and fieldAttrib_ have same count and order of fields
        fieldAttrib_.append(fieldAttrib);
        field->addItem(protocol->fieldData(i, AbstractProtocol::FieldName)
                                        .toString());
    }

    protocolId_ = protocol->protocolNumber();
    delete protocol;
}

void FindReplaceDialog::on_field_currentIndexChanged(int index)
{
    if (index < 0)
        return;

    QString fieldName = field->currentText();
    FieldAttrib fieldAttrib = fieldAttrib_.at(index);

    // Use heuristics to determine field type
    if (fieldAttrib.bitSize == 48) {
        findMask->setType(FieldEdit::kMacAddress);
        findValue->setType(FieldEdit::kMacAddress);
        replaceMask->setType(FieldEdit::kMacAddress);
        replaceValue->setType(FieldEdit::kMacAddress);
    } else if ((fieldAttrib.bitSize == 32)
            && (fieldName.contains(QRegularExpression(
                        "address|source|destination",
                        QRegularExpression::CaseInsensitiveOption)))) {
        findMask->setType(FieldEdit::kIp4Address);
        findValue->setType(FieldEdit::kIp4Address);
        replaceMask->setType(FieldEdit::kIp4Address);
        replaceValue->setType(FieldEdit::kIp4Address);
    } else if ((fieldAttrib.bitSize == 128)
            && (fieldName.contains(QRegularExpression(
                        "address|source|destination",
                        QRegularExpression::CaseInsensitiveOption)))) {
        findMask->setType(FieldEdit::kIp6Address);
        findValue->setType(FieldEdit::kIp6Address);
        replaceMask->setType(FieldEdit::kIp6Address);
        replaceValue->setType(FieldEdit::kIp6Address);
    } else {
        quint64 max = quint64(~0) >> (64-fieldAttrib.bitSize);
        qDebug("XXXXXX %s bitSize %d max %llx",
                qPrintable(field->currentText()),
                fieldAttrib.bitSize, max);

        findMask->setType(FieldEdit::kUInt64);
        findMask->setRange(0, max);
        findValue->setType(FieldEdit::kUInt64);
        findValue->setRange(0, max);

        replaceMask->setType(FieldEdit::kUInt64);
        replaceMask->setRange(0, max);
        replaceValue->setType(FieldEdit::kUInt64);
        replaceValue->setRange(0, max);
    }
}

void FindReplaceDialog::on_buttonBox_accepted()
{
    FieldAttrib fieldAttrib = fieldAttrib_.at(field->currentIndex());
    action_->protocolField = QString("%1 %2")
                                .arg(protocol->currentText())
                                .arg(field->currentText());
    action_->protocolNumber = protocolId_;
    action_->fieldIndex = fieldAttrib.index;
    action_->fieldBitSize = fieldAttrib.bitSize;

    if (fieldAttrib.bitSize == 128) { // IPv6 address
        if (matchAny->isChecked()) {
            action_->findMask.setValue(UInt128(0));
            action_->findValue.setValue(UInt128(0));
        } else {
            action_->findMask.setValue(
                        useFindMask->isChecked() ?
                            ipUtils::ip6StringToUInt128(findMask->text()) :
                            ~UInt128(0));
            action_->findValue.setValue(
                        ipUtils::ip6StringToUInt128(findValue->text()));
        }

        action_->replaceMask.setValue(
                        useReplaceMask->isChecked() ?
                            ipUtils::ip6StringToUInt128(replaceMask->text()) :
                            ~UInt128(0));
        action_->replaceValue.setValue(
                        ipUtils::ip6StringToUInt128(replaceValue->text()));
    } else { // everything else
        if (matchAny->isChecked()) {
            action_->findMask.setValue(0);
            action_->findValue.setValue(0);
        } else {
            action_->findMask.setValue(
                        useFindMask->isChecked() ?
                            findMask->text().toULongLong(nullptr, 0) :
                            ~quint64(0));
            action_->findValue.setValue(
                        findValue->text().toULongLong(nullptr, 0));
        }

        action_->replaceMask.setValue(
                        useReplaceMask->isChecked() ?
                            replaceMask->text().toULongLong(nullptr, 0) :
                            ~quint64(0));
        action_->replaceValue.setValue(QString::number(
                    replaceValue->text().toULongLong(nullptr, 0)));
    }

    action_->selectedStreamsOnly = selectedStreamsOnly->isChecked();
}

