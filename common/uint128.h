/*
Copyright (C) 2015 Srivats P.

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

#ifndef _UINT128_H
#define _UINT128_H

#include <QtGlobal>
#include <qendian.h>

class UInt128
{
public:
    UInt128();
    UInt128(quint64 hi, quint64 lo);

    quint8* toArray() const;

    UInt128 operator+(const UInt128 &other);
    UInt128 operator*(const uint &other);

private:
    quint64 hi_;
    quint64 lo_;
    quint8 array_[16]; 
};

inline UInt128::UInt128()
{
    // Do nothing - value will be garbage like any other uint
}

inline UInt128::UInt128(quint64 hi, quint64 lo)
{
    hi_ = hi;
    lo_ = lo;
}

inline quint8* UInt128::toArray() const
{
    *(quint64*)(array_ + 0) = qToBigEndian<quint64>(hi_);
    *(quint64*)(array_ + 8) = qToBigEndian<quint64>(lo_);

    return (quint8*)array_;
}

inline UInt128 UInt128::operator+(const UInt128 &other)
{
    UInt128 sum;

    sum.lo_ = lo_ + other.lo_;
    sum.hi_ = hi_ + other.hi_ + (sum.lo_ < lo_);

    return sum;
}

inline UInt128 UInt128::operator*(const uint &other)
{
    UInt128 product;
  
    // FIXME 
    product.hi_ = 0;
    product.lo_ = lo_ * other;

    return product;
}

#endif
