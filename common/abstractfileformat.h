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

#ifndef _ABSTRACT_FILE_FORMAT_H
#define _ABSTRACT_FILE_FORMAT_H

#include "protocol.pb.h"

#include <QObject>
#include <QString>

class AbstractFileFormat : public QObject
{
    Q_OBJECT 
public:
    AbstractFileFormat();
    virtual ~AbstractFileFormat();

    virtual bool openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error) = 0;
    virtual bool saveStreams(const OstProto::StreamConfigList streams, 
            const QString fileName, QString &error) = 0;

    static AbstractFileFormat* fileFormatFromFile(const QString fileName);
    static AbstractFileFormat* fileFormatFromType(const QString fileType);

    static QStringList supportedFileTypes();

#if 0
    bool isMyFileFormat(const QString fileName) = 0;
    bool isMyFileType(const QString fileType) = 0;
#endif
};

#endif

