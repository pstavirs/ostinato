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
#include <QValidator>

class IPv4AddressValidator : public QValidator
{
public:
    IPv4AddressValidator(QObject *parent = 0)
        : QValidator(parent)
    {
        _ip4ValidChars.setPattern("[0-9]{1,3}(.[0-9]{1,3}){0,4}");
    }
    ~IPv4AddressValidator() {}

    virtual QValidator::State validate(QString &input, int& /*pos*/) const
    {
        QValidator::State state;
        QHostAddress addr(input);

        qDebug("%s: %s", __FUNCTION__, qPrintable(input));

        if (addr.protocol() == QAbstractSocket::IPv4Protocol)
            state = Acceptable;
        else
            if (_ip4ValidChars.exactMatch(input))
                state = Intermediate;
            else
                state = Invalid;
        qDebug("%s(%d): %s, ", __FUNCTION__, state, qPrintable(input));
        return state;
    }
    virtual void fixup(QString &input) const
    {
        input.append(".0.0.0.0");
        QHostAddress addr(input);
        int len = input.size();

        qDebug("%s: %s", __FUNCTION__, qPrintable(input));

        while (addr.protocol() != QAbstractSocket::IPv4Protocol)
        {
            len--;
            Q_ASSERT(len >= 0);
            addr.setAddress(input.left(len));
        }

        input = addr.toString();
    }
private:
    QRegExp _ip4ValidChars;
};

#endif
