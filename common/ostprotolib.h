/*
Copyright (C) 2011 Srivats P.

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
#ifndef _OST_PROTO_LIB_H
#define _OST_PROTO_LIB_H

#include <QString>

class OstProtoLib
{
public:
    static void setExternalApplicationPaths(QString tsharkPath,
            QString gzipPath, QString diffPath, QString awkPath);

    static QString tsharkPath();
    static QString gzipPath();
    static QString diffPath();
    static QString awkPath();

private:
    static QString tsharkPath_;
    static QString gzipPath_;
    static QString diffPath_;
    static QString awkPath_;
};

#endif
