/*
Copyright (C) 2018 Srivats P.

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

#ifndef _X_LOCALE_H
#define _X_LOCALE_H

#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

class XLocale: public QLocale
{
public:
    double toDouble(const QString &s, bool *ok = Q_NULLPTR) const
    {
        QString s2 = s;
        return QLocale::toDouble(s2.remove(groupSeparator()), ok);
    }

    double toPacketsPerSecond(const QString &s, bool *ok = Q_NULLPTR) const
    {
        QString text = s;
        double multiplier = 0;
        QRegularExpression regex("[a-zA-Z/]+$");
        QRegularExpressionMatch match = regex.match(text);
        if (match.hasMatch()) {
            QString unit = match.captured(0).toCaseFolded();
            if ((unit == "mpps") || (unit == "m"))
                multiplier = 1e6;
            else if ((unit == "kpps") || (unit == "k"))
                multiplier = 1e3;
            else if (unit == "pps")
                multiplier = 1;

            if (multiplier)
                text.remove(regex);
        }

        if (multiplier == 0)
            multiplier = 1;

        return toDouble(text, ok) * multiplier;
    }

    double toBitsPerSecond(const QString &s, bool *ok = Q_NULLPTR) const
    {
        QString text = s;
        double multiplier = 0;
        QRegularExpression regex("[a-zA-Z/]+$");
        QRegularExpressionMatch match = regex.match(text);
        if (match.hasMatch()) {
            QString unit = match.captured(0).toCaseFolded();
            if ((unit == "gbps") || (unit == "gb/s") || (unit == "g"))
                multiplier = 1e9;
            else if ((unit == "mbps") || (unit == "mb/s") || (unit == "m"))
                multiplier = 1e6;
            else if ((unit == "kbps") || (unit == "kb/s") || (unit == "k"))
                multiplier = 1e3;
            else if ((unit == "bps") || (unit == "b/s"))
                multiplier = 1;

            if (multiplier)
                text.remove(regex);
        }

        if (multiplier == 0)
            multiplier = 1;

        return toDouble(text, ok) * multiplier;
    }

    QString toBitRateString(double bps) const
    {
        QString text;

        if (bps >= 1e9)
            return QObject::tr("%L1 Gbps").arg(bps/1e9, 0, 'f', 4);

        if (bps >= 1e6)
            return QObject::tr("%L1 Mbps").arg(bps/1e6, 0, 'f', 4);

        if (bps >= 1e3)
            return QObject::tr("%L1 Kbps").arg(bps/1e3, 0, 'f', 4);

        return QObject::tr("%L1 bps").arg(bps, 0, 'f', 4);
    }

    QString toTimeIntervalString(qint64 nanosecs) const
    {
        QString text;

        if (nanosecs >= 1e9)
            return QObject::tr("%L1 s").arg(nanosecs/1e9, 0, 'f', 3);

        if (nanosecs >= 1e6)
            return QObject::tr("%L1 ms").arg(nanosecs/1e6, 0, 'f', 3);

        if (nanosecs >= 1e3)
            return QObject::tr("%L1 us").arg(nanosecs/1e3, 0, 'f', 3);

        return QObject::tr("%L1 ns").arg(nanosecs);
    }
};

#endif

