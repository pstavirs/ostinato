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

    // Keep things simple and don't use mask(s) (default)
    useFindMask->setChecked(false);
    useReplaceMask->setChecked(false);

    // TODO: remove combo protocols, sample, userscript
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
        // FIXME: do we need max, since we already have bitSize?
        fieldAttrib.max = quint64(~0) >> (64-bitSize); // min is always 0

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
        findValue->setType(FieldEdit::kMacAddress);
        replaceValue->setType(FieldEdit::kMacAddress);
    } else if ((fieldAttrib.bitSize == 32)
            && (fieldName.contains(QRegularExpression(
                        "address|source|destination",
                        QRegularExpression::CaseInsensitiveOption)))) {
        findValue->setType(FieldEdit::kIp4Address);
        replaceValue->setType(FieldEdit::kIp4Address);
    } else if ((fieldAttrib.bitSize == 128)
            && (fieldName.contains(QRegularExpression(
                        "address|source|destination",
                        QRegularExpression::CaseInsensitiveOption)))) {
        findValue->setType(FieldEdit::kIp6Address);
        replaceValue->setType(FieldEdit::kIp6Address);
    } else {
        qDebug("XXXXXX %s bitSize %d max %llx",
                qPrintable(field->currentText()),
                fieldAttrib.bitSize, fieldAttrib.max);

        findValue->setType(FieldEdit::kUInt64);
        findValue->setRange(0, fieldAttrib.max);

        replaceValue->setType(FieldEdit::kUInt64);
        replaceValue->setRange(0, fieldAttrib.max);
    }
}

void FindReplaceDialog::on_buttonBox_accepted()
{
    FieldAttrib fieldAttrib = fieldAttrib_.at(field->currentIndex());
    action_->protocolNumber = protocolId_;
    action_->fieldIndex = fieldAttrib.index;
    action_->fieldBitSize = fieldAttrib.bitSize;

    // TODO: Change <= 64 to uint64 instead of string
    // XXX: All find/replace value/mask QVariants are set to
    // 64-bit decimal number encoded as string
    //   - The action user is expected to convert to appropriate type
    //     (fieldBitSize is included as a hint)
    //   - QVariant can only do decimal conversions (not hex)

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
            action_->findMask.setValue(QString("0"));
            action_->findValue.setValue(QString("0"));
        } else {
            action_->findMask.setValue(QString::number(
                        useFindMask->isChecked() ?
                            findMask->text().toULongLong(nullptr, BASE_HEX) :
                            quint64(~0)));
            action_->findValue.setValue(QString::number(
                        findValue->text().toULongLong(nullptr, 0)));
        }

        action_->replaceMask.setValue(QString::number(
                        useReplaceMask->isChecked() ?
                            replaceMask->text().toULongLong(nullptr, BASE_HEX) :
                            quint64(~0)));
        action_->replaceValue.setValue(QString::number(
                    replaceValue->text().toULongLong(nullptr, 0)));
    }

    action_->selectedStreamsOnly = selectedStreamsOnly->isChecked();
}

