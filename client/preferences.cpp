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

#include "preferences.h"

#include "../common/ostprotolib.h"
#include "settings.h"

#include <QFileDialog>

Preferences::Preferences()
{
    Q_ASSERT(appSettings);

    setupUi(this);

    wiresharkPathEdit->setText(appSettings->value(kWiresharkPathKey, 
            kWiresharkPathDefaultValue).toString());
    tsharkPathEdit->setText(appSettings->value(kTsharkPathKey, 
            kTsharkPathDefaultValue).toString());
    gzipPathEdit->setText(appSettings->value(kGzipPathKey, 
            kGzipPathDefaultValue).toString());
    diffPathEdit->setText(appSettings->value(kDiffPathKey, 
            kDiffPathDefaultValue).toString());
    awkPathEdit->setText(appSettings->value(kAwkPathKey, 
            kAwkPathDefaultValue).toString());
}

Preferences::~Preferences()
{
}

void Preferences::accept()
{
    appSettings->setValue(kWiresharkPathKey, wiresharkPathEdit->text());
    appSettings->setValue(kTsharkPathKey, tsharkPathEdit->text());
    appSettings->setValue(kGzipPathKey, gzipPathEdit->text());
    appSettings->setValue(kDiffPathKey, diffPathEdit->text());
    appSettings->setValue(kAwkPathKey, awkPathEdit->text());

    OstProtoLib::setExternalApplicationPaths(
        appSettings->value(kTsharkPathKey, kTsharkPathDefaultValue).toString(),
        appSettings->value(kGzipPathKey, kGzipPathDefaultValue).toString(),
        appSettings->value(kDiffPathKey, kDiffPathDefaultValue).toString(),
        appSettings->value(kAwkPathKey, kAwkPathDefaultValue).toString());

    QDialog::accept();
}

void Preferences::on_wiresharkPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate Wireshark",
            wiresharkPathEdit->text()); 

    if (!path.isEmpty())
        wiresharkPathEdit->setText(path);
}

void Preferences::on_tsharkPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate tshark",
            tsharkPathEdit->text()); 

    if (!path.isEmpty())
        tsharkPathEdit->setText(path);
}

void Preferences::on_gzipPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate gzip",
            gzipPathEdit->text()); 

    if (!path.isEmpty())
        gzipPathEdit->setText(path);
}

void Preferences::on_diffPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate diff",
            diffPathEdit->text()); 

    if (!path.isEmpty())
        diffPathEdit->setText(path);
}

void Preferences::on_awkPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate awk",
            awkPathEdit->text()); 

    if (!path.isEmpty())
        awkPathEdit->setText(path);
}
