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

#include "mandatoryfieldsgroup.h"

// No need for QDateEdit, QSpinBox, etc., since these always return values
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

void MandatoryFieldsGroup::add(QWidget *widget)
{
    if (!widgets_.contains(widget)) {
        if (widget->inherits("QCheckBox"))
            connect(qobject_cast<QCheckBox*>(widget),
                    SIGNAL(clicked()),
                    this, SLOT(changed()));
        else if (widget->inherits("QComboBox"))
            connect(qobject_cast<QComboBox*>(widget),
                    SIGNAL(highlighted(int)),
                    this, SLOT(changed()));
        else if (widget->inherits("QLineEdit"))
            connect(qobject_cast<QLineEdit*>(widget),
                    SIGNAL(textChanged(const QString&)),
                    this, SLOT(changed()));
        else {
            qWarning("MandatoryFieldsGroup: unsupported class %s",
                     widget->metaObject()->className());
            return;
        }

        widgets_.append(widget);
        changed();
    }
}

void MandatoryFieldsGroup::remove(QWidget *widget)
{
    widgets_.removeAll(widget);

    changed();
}

void MandatoryFieldsGroup::setSubmitButton(QPushButton *button)
{
    if (submitButton_ && submitButton_ != button)
        submitButton_->setEnabled(true);
    submitButton_ = button;

    changed();
}

void MandatoryFieldsGroup::changed()
{
    if (!submitButton_)
        return;

    bool enable = true;
    for (auto widget : widgets_) {
        // Invisible mandatory widgets are treated as non-mandatory
        if (!widget->isVisible())
            continue;

        if (widget->inherits("QCheckBox")) {
            // Makes sense only for tristate checkbox
            auto checkBox = qobject_cast<const QCheckBox*>(widget);
            if (checkBox->checkState() == Qt::PartiallyChecked) {
                enable = false;
                break;
            } else
                continue;
        }

        if (widget->inherits("QComboBox")) {
            auto comboBox = qobject_cast<const QComboBox*>(widget);
            if (comboBox->currentText().isEmpty()) {
                enable = false;
                break;
            } else
                continue;
        }

        if (widget->inherits("QLineEdit")) {
            auto lineEdit = qobject_cast<const QLineEdit*>(widget);
            if (lineEdit->text().isEmpty()
                    || !lineEdit->hasAcceptableInput()) {
                enable = false;
                break;
            } else
                continue;
        }
    }

    submitButton_->setEnabled(enable);
}

void MandatoryFieldsGroup::clear()
{
    widgets_.clear();

    if (submitButton_) {
        submitButton_->setEnabled(true);
        submitButton_ = nullptr;
    }
}



