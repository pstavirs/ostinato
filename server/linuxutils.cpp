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

#include "linuxutils.h"

#include <QFile>
#include <QTextStream>

// Reads a text file (size < 4K) and returns content as a string
// A terminating \n will be removed
// There's no way to distinguish an empty file and error while reading
QString readTextFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Can't read %s", qUtf8Printable(fileName));
        return QString();
    }

    if (file.size() > 4096) {
        qWarning("Can't read %s - too large (%lld)",
                 qUtf8Printable(fileName), file.size());
        return QString();
    }

    QTextStream in(&file);
    QString text = in.readAll();
    file.close();

    if (text.endsWith('\n'))
        text.chop(1);

    return text;
}

// Reads value from /sys/class/net/<device>/<attrib-path>
// and returns as string
// XXX: reading from sysfs is discouraged
QString sysfsAttrib(const char *device, const char *attribPath)
{
    return readTextFile(QString("/sys/class/net/%1/%2")
                            .arg(device).arg(attribPath));
}

// Convenience overload
QString sysfsAttrib(QString device, const char *attribPath)
{
    return sysfsAttrib(qPrintable(device), attribPath);
}
