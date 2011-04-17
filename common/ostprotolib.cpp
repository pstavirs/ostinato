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

#include "ostprotolib.h"

QString OstProtoLib::tsharkPath_;
QString OstProtoLib::gzipPath_;
QString OstProtoLib::diffPath_;
QString OstProtoLib::awkPath_;

void OstProtoLib::setExternalApplicationPaths(QString tsharkPath, 
        QString gzipPath, QString diffPath, QString awkPath)
{
    tsharkPath_ = tsharkPath;
    gzipPath_ = gzipPath;
    diffPath_ = diffPath;
    awkPath_ = awkPath;
}

QString OstProtoLib::tsharkPath()
{
    return tsharkPath_;
}

QString OstProtoLib::gzipPath()
{
    return gzipPath_;
}

QString OstProtoLib::diffPath()
{
    return diffPath_;
}

QString OstProtoLib::awkPath()
{
    return awkPath_;
}

