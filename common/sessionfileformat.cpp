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

#include "sessionfileformat.h"

#include "ossnfileformat.h"

#include <QStringList>

SessionFileFormat::SessionFileFormat()
{
    stop_ = false;
}

SessionFileFormat::~SessionFileFormat()
{
}

QDialog* SessionFileFormat::openOptionsDialog()
{
    return NULL;
}

QDialog* SessionFileFormat::saveOptionsDialog()
{
    return NULL;
}

QStringList SessionFileFormat::supportedFileTypes(Operation op)
{
    QStringList fileTypes;

    fileTypes << "Ostinato Session (*.ossn)";

    if (op == kOpenFile)
        fileTypes << "All files (*)";

    return fileTypes;
}

void SessionFileFormat::openAsync(const QString fileName,
        OstProto::SessionContent &session, QString &error)
{
    fileName_ = fileName;
    openSession_ = &session;
    error_ = &error;
    op_ = kOpenFile;
    stop_ = false;

    start();
}

void SessionFileFormat::saveAsync(
        const OstProto::SessionContent &session,
        const QString fileName, QString &error)
{
    saveSession_ = &session;
    fileName_ = fileName;
    error_ = &error;
    op_ = kSaveFile;
    stop_ = false;

    start();
}

bool SessionFileFormat::result()
{
    return result_;
}

SessionFileFormat* SessionFileFormat::fileFormatFromFile(
        const QString fileName)
{
    if (ossnFileFormat.isMyFileFormat(fileName))
        return &ossnFileFormat;

    return NULL;
}

SessionFileFormat* SessionFileFormat::fileFormatFromType(
        const QString fileType)
{

    if (ossnFileFormat.isMyFileType(fileType))
        return &ossnFileFormat;

    return NULL;
}

void SessionFileFormat::cancel()
{
    stop_ = true;
}

void SessionFileFormat::run()
{
    if (op_ == kOpenFile)
        result_ = open(fileName_, *openSession_, *error_);
    else if (op_ == kSaveFile)
        result_ = save(*saveSession_, fileName_, *error_);
}
