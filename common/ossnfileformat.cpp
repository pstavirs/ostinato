/*
Copyright (C) 2016 Srivats P.

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

#include "ossnfileformat.h"

bool OssnFileFormat::open(const QString fileName,
        OstProto::SessionContent &session, QString &error)
{
    // TODO
    return false;
}

bool OssnFileFormat::save(const OstProto::SessionContent &session,
        const QString fileName, QString &error)
{
    // TODO
    return false;
}

bool OssnFileFormat::isMyFileFormat(const QString fileName)
{
    // TODO
    return true;
}

bool OssnFileFormat::isMyFileType(const QString fileType)
{
    // TODO
    return true;
}
