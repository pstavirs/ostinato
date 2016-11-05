/*
Copyright (C) 2010-2016 Srivats P.

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

#ifndef _B_SWAP_H
#define _B_SWAP_H

#include <QtGlobal>

static inline quint32 swap32(quint32 val)
{
    return (((val >> 24) & 0x000000FF) |
            ((val >> 16) & 0x0000FF00) |
            ((val << 16) & 0x00FF0000) |
            ((val << 24) & 0xFF000000));
}

static inline quint16 swap16(quint16 val)
{
    return (((val >> 8) & 0x00FF) |
            ((val << 8) & 0xFF00));
}

#endif
