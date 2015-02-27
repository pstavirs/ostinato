/*
Copyright (C) 2010 Srivats P.

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

/*!
 * \todo Remove this class
 */

#include <qendian.h>
#include <QHostAddress>

#include "stream.h"
#include "../common/protocollistiterator.h"
#include "../common/abstractprotocol.h"

Stream::Stream()
{
    //mId = 0xFFFFFFFF;
    setEnabled(true);
}

Stream::~Stream()
{
}

void Stream::loadProtocolWidgets()
{
    qWarning("%s: DOES NOTHING", __PRETTY_FUNCTION__);
    return;
}

void Stream::storeProtocolWidgets()
{
    qWarning("%s: DOES NOTHING", __PRETTY_FUNCTION__);
    return;
}
