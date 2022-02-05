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
#include "thememanager.h"

#include <QFileDialog>
#include <QtGlobal>
#include <QXmlStreamReader>

#if defined(Q_OS_WIN32)
QString kGzipPathDefaultValue;
QString kDiffPathDefaultValue;
QString kAwkPathDefaultValue;
#endif

QString kUserDefaultValue;

Preferences::Preferences()
{
    Q_ASSERT(appSettings);

    setupUi(this);

    // Program paths
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

    // Theme
    theme->addItems(ThemeManager::instance()->themeList());
    theme->setCurrentText(appSettings->value(kThemeKey).toString());

    // TODO(only if required): kUserKey
}

Preferences::~Preferences()
{
}

void Preferences::initDefaults()
{

#if defined(Q_OS_WIN32)
    kGzipPathDefaultValue = QApplication::applicationDirPath() + "/gzip.exe";
    kDiffPathDefaultValue = QApplication::applicationDirPath() + "/diff.exe";
    kAwkPathDefaultValue = QApplication::applicationDirPath() + "/gawk.exe";
#endif

    // Read default username from the environment
#ifdef Q_OS_WIN32
    kUserDefaultValue = QString(qgetenv("USERNAME").constData());
#else
    kUserDefaultValue = QString(qgetenv("USER").constData());
#endif
    qDebug("current user <%s>", qPrintable(kUserDefaultValue));
}

void Preferences::accept()
{
    // Program paths
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

    // Theme
    ThemeManager::instance()->setTheme(theme->currentText());

    QDialog::accept();
}

void Preferences::on_wiresharkPathButton_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(0, "Locate Wireshark",
            wiresharkPathEdit->text()); 
#ifdef Q_OS_MAC
    // Find executable inside app bundle using Info.plist
    if (!path.isEmpty() && path.endsWith(".app")) {
        QFile plist(path+"/Contents/Info.plist");
        plist.open(QIODevice::ReadOnly);
        QXmlStreamReader xml(&plist);

        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()
                    && (xml.name() == "key")
                    && (xml.readElementText() == "CFBundleExecutable")) {
                xml.readNext(); // </key>
                xml.readNext(); // <string>
                if (xml.isStartElement() && (xml.name() == "string"))
                    path = path+"/Contents/MacOs/"+xml.readElementText();
                break;
            }
            if (xml.hasError())
                qDebug("%lld:%lld Error reading Info.plist: %s",
                       xml.lineNumber(), xml.columnNumber(),
                       qPrintable(xml.errorString()));
        }
    }
#endif

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
