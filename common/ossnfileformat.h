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

#ifndef _OSSN_FILE_FORMAT_H
#define _OSSN_FILE_FORMAT_H

#include "nativefileformat.h"
#include "sessionfileformat.h"

class OssnFileFormat : public SessionFileFormat, public NativeFileFormat
{
public:
    OssnFileFormat();

    virtual bool open(const QString fileName,
            OstProto::SessionContent &session, QString &error);
    virtual bool save(const OstProto::SessionContent &session,
            const QString fileName, QString &error);

    virtual bool isMyFileFormat(const QString fileName);
    virtual bool isMyFileType(const QString fileType);
};

extern OssnFileFormat ossnFileFormat;

#endif

