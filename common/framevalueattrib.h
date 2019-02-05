/*
Copyright (C) 2019 Srivats P.

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

#ifndef _FRAME_VALUE_ATTRIB_H
#define _FRAME_VALUE_ATTRIB_H

#include <QFlags>

struct FrameValueAttrib
{
    enum ErrorFlag {
        UnresolvedSrcMacError = 0x1,
        UnresolvedDstMacError = 0x2,
    };
    Q_DECLARE_FLAGS(ErrorFlags, ErrorFlag);
    ErrorFlags errorFlags{0};
    // TODO?: int len;
    
    FrameValueAttrib& operator+=(const FrameValueAttrib& rhs) {
        errorFlags |= rhs.errorFlags;
        return *this;
    }
    FrameValueAttrib operator+(const FrameValueAttrib& rhs) {
        FrameValueAttrib result = *this;
        result += rhs;
        return result;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FrameValueAttrib::ErrorFlags)

#endif

