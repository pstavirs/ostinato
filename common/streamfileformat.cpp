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

#include "streamfileformat.h"

#include "ostmfileformat.h"
#include "pcapfileformat.h"
#include "pdmlfileformat.h"
#include "pythonfileformat.h"

#include <QStringList>

StreamFileFormat::StreamFileFormat()
{
    stop_ = false;
}

StreamFileFormat::~StreamFileFormat()
{
}

QDialog* StreamFileFormat::openOptionsDialog()
{
    return NULL;
}

QDialog* StreamFileFormat::saveOptionsDialog()
{
    return NULL;
}

QStringList StreamFileFormat::supportedFileTypes(Operation op)
{
    QStringList fileTypes;

    fileTypes
        << "Ostinato (*.ostm)"
        << "PCAP (*.pcap)"
        << "PDML (*.pdml)";

    if (op == kSaveFile)
        fileTypes << "PythonScript (*.py)";
    else if (op == kOpenFile)
        fileTypes << "All files (*)";

    return fileTypes;
}

void StreamFileFormat::openAsync(const QString fileName,
        OstProto::StreamConfigList &streams, QString &error)
{
    fileName_ = fileName;
    openStreams_ = &streams;
    error_ = &error;
    op_ = kOpenFile;
    stop_ = false;

    start();
}

void StreamFileFormat::saveAsync(
        const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    saveStreams_ = streams;
    fileName_ = fileName;
    error_ = &error;
    op_ = kSaveFile;
    stop_ = false;

    start();
}

bool StreamFileFormat::result()
{
    return result_;
}

StreamFileFormat* StreamFileFormat::fileFormatFromFile(
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

StreamFileFormat* StreamFileFormat::fileFormatFromType(
        const QString fileType)
{

    if (fileFormat.isMyFileType(fileType))
        return &fileFormat;

    if (pdmlFileFormat.isMyFileType(fileType))
        return &pdmlFileFormat;

    if (pcapFileFormat.isMyFileType(fileType))
        return &pcapFileFormat;

    if (pythonFileFormat.isMyFileType(fileType))
        return &pythonFileFormat;

    return NULL;
}

void StreamFileFormat::cancel()
{
    stop_ = true;
}

void StreamFileFormat::run()
{
    if (op_ == kOpenFile)
        result_ = open(fileName_, *openStreams_, *error_);
    else if (op_ == kSaveFile)
        result_ = save(saveStreams_, fileName_, *error_);
}
