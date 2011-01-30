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
#ifndef _FILE_FORMAT_H
#define _FILE_FORMAT_H

#include "fileformat.pb.h"

#include <QString>
#include <QObject>

class FileFormat : public QObject
{
    Q_OBJECT 
public:
    FileFormat();
    ~FileFormat();

    bool openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error);
    bool saveStreams(const OstProto::StreamConfigList streams, 
            const QString fileName, QString &error);

private:
    static const int kFileMagicSize = 12;
    static const int kFileChecksumSize = 5;
    static const int kFileMinSize = kFileMagicSize + kFileChecksumSize;

    static const int kFileMagicOffset = 0;
    static const int kFileMetaDataOffset = kFileMagicSize;

    static const std::string kFileMagicValue;
    
    // Native file format version
    static const uint kFileFormatVersionMajor = 0;
    static const uint kFileFormatVersionMinor = 1;
    static const uint kFileFormatVersionRevision = 3;

    void initFileMetaData(OstProto::FileMetaData &metaData);
};

extern FileFormat fileFormat;

#endif
