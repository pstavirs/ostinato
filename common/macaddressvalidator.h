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

#ifndef _MAC_ADDRESS_VALIDATOR_H
#define _MAC_ADDRESS_VALIDATOR_H

#include <QRegExpValidator>

class MacAddressValidator : public QRegExpValidator
{
public:
    MacAddressValidator(QObject *parent = 0)
        : QRegExpValidator(parent)
    {
        // Allow : or - as separator
        setRegExp(QRegExp("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}"));
    }
    ~MacAddressValidator() {}
};

#endif
