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

#include <QHash>

#include <QtGlobal>
#include <qendian.h>

class UInt128
{
public:
    UInt128();
    UInt128(int lo);
    UInt128(quint64 hi, quint64 lo);
    UInt128(quint8 *value);

    quint64 hi64() const;
    quint64 lo64() const;
    quint8* toArray() const;

    bool operator!() const;
    bool operator==(const UInt128 &other) const;
    bool operator!=(const UInt128 &other) const;
    UInt128 operator+(const UInt128 &other) const;
    UInt128 operator*(const uint &other) const;
    UInt128 operator<<(const int &shift) const;
    UInt128 operator~() const;
    UInt128 operator&(const UInt128 &other) const;
    UInt128 operator|(const UInt128 &other) const;

private:
    quint64 hi_;
    quint64 lo_;
    quint8 array_[16]; 
};

inline UInt128::UInt128()
{
    // Do nothing - value will be garbage like any other uint
}

inline UInt128::UInt128(int lo)
{
    hi_ = 0;
    lo_ = lo;
}

inline UInt128::UInt128(quint64 hi, quint64 lo)
{
    hi_ = hi;
    lo_ = lo;
}

inline UInt128::UInt128(quint8 *value)
{
    hi_ = (quint64(value[0]) << 56)
        | (quint64(value[1]) << 48)
        | (quint64(value[2]) << 40)
        | (quint64(value[3]) << 32)
        | (quint64(value[4]) << 24)
        | (quint64(value[5]) << 16)
        | (quint64(value[6]) <<  8)
        | (quint64(value[7]) <<  0);

    lo_ = (quint64(value[ 8]) << 56)
        | (quint64(value[ 9]) << 48)
        | (quint64(value[10]) << 40)
        | (quint64(value[11]) << 32)
        | (quint64(value[12]) << 24)
        | (quint64(value[13]) << 16)
        | (quint64(value[14]) <<  8)
        | (quint64(value[15]) <<  0);
}

inline quint64 UInt128::hi64() const
{
    return hi_;
}

inline quint64 UInt128::lo64() const
{
    return lo_;
}

inline quint8* UInt128::toArray() const
{
    qToBigEndian(hi_, const_cast<uchar*>(array_ + 0));
    qToBigEndian(lo_, const_cast<uchar*>(array_ + 8));

    return (quint8*)array_;
}

inline bool UInt128::operator!() const
{
    return (hi_ == 0) && (lo_ == 0);
}

inline bool UInt128::operator==(const UInt128 &other) const
{
    return ((hi_ == other.hi_) && (lo_ == other.lo_));
}

inline bool UInt128::operator!=(const UInt128 &other) const
{
    return ((hi_ != other.hi_) || (lo_ != other.lo_));
}

inline UInt128 UInt128::operator+(const UInt128 &other) const
{
    UInt128 sum;

    sum.lo_ = lo_ + other.lo_;
    sum.hi_ = hi_ + other.hi_ + (sum.lo_ < lo_);

    return sum;
}

inline UInt128 UInt128::operator*(const uint &other) const
{
    UInt128 product;
  
    // FIXME 
    product.hi_ = 0;
    product.lo_ = lo_ * other;

    return product;
}

inline UInt128 UInt128::operator<<(const int &shift) const
{
    UInt128 shifted;

    if (shift < 64)
        return UInt128((hi_<<shift) | (lo_>>(64-shift)), lo_ << shift);

    return UInt128(hi_<<(shift-64), 0);
}

inline UInt128 UInt128::operator~() const
{
    return UInt128(~hi_, ~lo_);
}

inline UInt128 UInt128::operator&(const UInt128 &other) const
{
    return UInt128(hi_ & other.hi_, lo_ & other.lo_);
}

inline UInt128 UInt128::operator|(const UInt128 &other) const
{
    return UInt128(hi_ | other.hi_, lo_ | other.lo_);
}

#if QT_VERSION >= 0x050700
template <> inline UInt128 qFromBigEndian<UInt128>(const void *src)
#else
template <> inline UInt128 qFromBigEndian<UInt128>(const uchar *src)
#endif
{
    quint64 hi, lo;

    hi = qFromBigEndian<quint64>(src);
    lo = qFromBigEndian<quint64>((uchar*)src+8);

    return UInt128(hi, lo);
}

template <> inline UInt128 qToBigEndian<UInt128>(const UInt128 src)
{
    quint64 hi, lo;

    hi = qToBigEndian<quint64>(src.hi64());
    lo = qToBigEndian<quint64>(src.lo64());

    return UInt128(hi, lo);
}

inline uint qHash(const UInt128 &key)
{
    return qHash(key.hi64()) ^ qHash(key.lo64());
}

#endif
