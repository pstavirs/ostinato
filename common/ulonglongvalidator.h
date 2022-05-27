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

#ifndef _ULONGLONG_VALIDATOR_H
#define _ULONGLONG_VALIDATOR_H

#include <QValidator>

class ULongLongValidator : public QValidator
{
public:
    ULongLongValidator(QObject *parent = 0)
        : QValidator(parent)
    {
    }
    ULongLongValidator(qulonglong min, qulonglong max, QObject *parent = 0)
        : QValidator(parent)
    {
        setRange(min, max);
    }
    ~ULongLongValidator() {}

    void setRange(qulonglong min, qulonglong max)
    {
        min_ = min;
        max_ = max;
    }

    virtual QValidator::State validate(QString &input, int& /*pos*/) const
    {
        if (input.isEmpty())
            return Intermediate;

        if (input.compare("0x", Qt::CaseInsensitive) == 0)
            return Intermediate;

        bool isOk;
        qulonglong v = input.toULongLong(&isOk, 0);

        //qDebug("input: %s, ok: %d, %llu", qPrintable(input), isOk, v);
        if (!isOk)
            return Invalid;

        if (v > max_)
            return Invalid;

        if (v < min_)
            return Intermediate;

        return Acceptable;
    }

    virtual void fixup(QString &input) const
    {
        int dummyPos = 0;
        State state = validate(input, dummyPos);

        if (state == Acceptable)
            return;

        input.setNum(min_);
    }

private:
    qulonglong min_{0};
    qulonglong max_{~0ULL};
};

#endif
