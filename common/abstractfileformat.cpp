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

#include "abstractfileformat.h"

#include "fileformat.h"
#include "pcapfileformat.h"
#include "pdmlfileformat.h"

#include <QStringList>

AbstractFileFormat::AbstractFileFormat()
{
}

AbstractFileFormat::~AbstractFileFormat()
{
}

QStringList AbstractFileFormat::supportedFileTypes()
{
    return QStringList()
        << "Ostinato (*)"
        << "PCAP (*)"
        << "PDML (*.pdml)";
}
AbstractFileFormat* AbstractFileFormat::fileFormatFromFile(
        const QString fileName)
{

    if (fileFormat.isMyFileFormat(fileName))
        return &fileFormat;

    if (pdmlFileFormat.isMyFileFormat(fileName))
        return &pdmlFileFormat;

    if (pcapFileFormat.isMyFileFormat(fileName))
        return &pcapFileFormat;

    return NULL;
}

AbstractFileFormat* AbstractFileFormat::fileFormatFromType(
        const QString fileType)
{

    if (fileFormat.isMyFileType(fileType))
        return &fileFormat;

    if (pdmlFileFormat.isMyFileType(fileType))
        return &pdmlFileFormat;

    if (pcapFileFormat.isMyFileType(fileType))
        return &pcapFileFormat;

    return NULL;
}
