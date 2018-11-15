/*
Copyright (C) 2018 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software FoundatiTosDscpWidget::on, either versiTosDscpWidget::on 3 of the License, or
(at your optiTosDscpWidget::on) any later versiTosDscpWidget::on.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
alTosDscpWidget::ong with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "tosdscp.h"

TosDscpWidget::TosDscpWidget(QWidget *parent)
    : QWidget(parent)
{
    codePoints_.insert("cs0", 0);
    codePoints_.insert("cs1", 8);
    codePoints_.insert("cs2", 16);
    codePoints_.insert("cs3", 24);
    codePoints_.insert("cs4", 32);
    codePoints_.insert("cs5", 40);
    codePoints_.insert("cs6", 48);
    codePoints_.insert("cs7", 56);
    codePoints_.insert("af11", 10);
    codePoints_.insert("af12", 12);
    codePoints_.insert("af13", 14);
    codePoints_.insert("af21", 18);
    codePoints_.insert("af22", 20);
    codePoints_.insert("af23", 22);
    codePoints_.insert("af31", 26);
    codePoints_.insert("af32", 28);
    codePoints_.insert("af33", 30);
    codePoints_.insert("af41", 34);
    codePoints_.insert("af42", 36);
    codePoints_.insert("af43", 38);
    codePoints_.insert("ef", 46);
    codePoints_.insert("voice-admit", 44);

    setupUi(this);
    dscp->addItems(codePoints_.keys());

    connect(precedence, SIGNAL(activated(int)), SLOT(setTosValue()));
    connect(lowDelay, SIGNAL(clicked(bool)), SLOT(setTosValue()));
    connect(highThroughput, SIGNAL(clicked(bool)), SLOT(setTosValue()));
    connect(highReliability, SIGNAL(clicked(bool)), SLOT(setTosValue()));

    connect(dscp, SIGNAL(activated(int)), SLOT(setDscpValue()));
    connect(ecn, SIGNAL(activated(int)), SLOT(setDscpValue()));

    connect(customValue, SIGNAL(valueChanged(int)), SLOT(setValue(int)));
}

int TosDscpWidget::value()
{
    return customValue->value() & 0xff;
}

void TosDscpWidget::setValue(int value)
{
    value &= 0xff;
    qDebug("value %02x", value);
    customValue->blockSignals(true);
    customValue->setValue(value); // avoid signal-slot loop
    customValue->blockSignals(false);

    dscp->setCurrentIndex(codePoints_.values().indexOf(value >> 2));
    ecn->setCurrentIndex(value & 0x03);

    precedence->setCurrentIndex(value >> 5);
    lowDelay->setChecked(value & 0x10);
    highThroughput->setChecked(value & 0x08);
    highReliability->setChecked(value & 0x04);
}

void TosDscpWidget::setTosValue()
{
    qDebug("prec %d", precedence->currentIndex());
    qDebug("delay %d", lowDelay->isChecked() ? 1 : 0);
    setValue((precedence->currentIndex() << 5)
           | (lowDelay->isChecked() ? 0x10 : 0x00)
           | (highThroughput->isChecked() ? 0x08 : 0x00)
           | (highReliability->isChecked() ? 0x04 : 0x00)
           | 0x00);
}

void TosDscpWidget::setDscpValue()
{
    setValue((codePoints_.value(dscp->currentText()) << 2)
           | ecn->currentIndex());
}
