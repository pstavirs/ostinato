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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QSettings>
#include <QString>

extern QSettings *appSettings;

const QString kWiresharkPathKey("WiresharkPath");
#if defined(Q_OS_WIN32)
const QString kWiresharkPathDefaultValue(
        "C:/Program Files/Wireshark/wireshark.exe");
#elif defined(Q_OS_MAC)
const QString kWiresharkPathDefaultValue(
        "/Applications/Wireshark.app/Contents/Resources/bin/wireshark");
#else
const QString kWiresharkPathDefaultValue("/usr/bin/wireshark");
#endif

const QString kTsharkPathKey("TsharkPath");
#if defined(Q_OS_WIN32)
const QString kTsharkPathDefaultValue(
        "C:/Program Files/Wireshark/tshark.exe");
#elif defined(Q_OS_MAC)
const QString kTsharkPathDefaultValue(
        "/Applications/Wireshark.app/Contents/Resources/bin/tshark");
#else
const QString kTsharkPathDefaultValue("/usr/bin/tshark");
#endif

const QString kGzipPathKey("GzipPath");
#if defined(Q_OS_WIN32)
extern QString kGzipPathDefaultValue;
#elif defined(Q_OS_MAC)
const QString kGzipPathDefaultValue("/usr/bin/gzip");
#else
const QString kGzipPathDefaultValue("/usr/bin/gzip");
#endif

const QString kDiffPathKey("DiffPath");
#if defined(Q_OS_WIN32)
extern QString kDiffPathDefaultValue;
#elif defined(Q_OS_MAC)
const QString kDiffPathDefaultValue("/usr/bin/diff");
#else
const QString kDiffPathDefaultValue("/usr/bin/diff");
#endif

const QString kAwkPathKey("AwkPath");
#if defined(Q_OS_WIN32)
extern QString kAwkPathDefaultValue;
#elif defined(Q_OS_MAC)
const QString kAwkPathDefaultValue("/usr/bin/awk");
#else
const QString kAwkPathDefaultValue("/usr/bin/awk");
#endif

#endif


