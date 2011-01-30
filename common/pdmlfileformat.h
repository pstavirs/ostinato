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
#ifndef _PDML_FILE_FORMAT_H
#define _PDML_FILE_FORMAT_H

#include "protocol.pb.h"

#include <QObject>

class PdmlParser;

class PdmlFileFormat : public QObject
{
    Q_OBJECT

public:
    PdmlFileFormat();
    ~PdmlFileFormat();

    bool openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error);
    bool saveStreams(const OstProto::StreamConfigList streams, 
            const QString fileName, QString &error);
};

extern PdmlFileFormat pdmlFileFormat;

#endif
