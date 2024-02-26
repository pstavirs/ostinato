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

#ifndef _IPV4_ADDRESS_VALIDATOR_H
#define _IPV4_ADDRESS_VALIDATOR_H

#include <QHostAddress>
#include <QRegularExpressionValidator>

class IPv4AddressValidator : public QRegularExpressionValidator
{
public:
    IPv4AddressValidator(QObject *parent = 0)
        : QRegularExpressionValidator(
                QRegularExpression(
                    "((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)(\\.(?!$)|$)){4}"),
                parent)
    {
    }

    virtual void fixup(QString &input) const
    {
        QStringList bytes = input.split('.', Qt::SkipEmptyParts);

        while (bytes.count() < 4)
            bytes.append("0");

        input = bytes.join('.');
    }
};

#endif
