/*
Copyright (C) 2010, 2016 Srivats P.

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
#ifndef _NATIVE_FILE_FORMAT_H
#define _NATIVE_FILE_FORMAT_H

/*
 * This file contains helper functions for the native file format
 * defined in fileformat.proto
 *
 * The actual file format classes - (Ostm)FileFormat and OssnFileFormat
 * use multiple inheritance from the abstract interface class and this
 * helper class
 *
 * The primary reason for the existence of this class is to have a common
 * code for dealing with native file formats
 */

#include "fileformat.pb.h"

#include <QString>

class NativeFileFormat
{
public:
    NativeFileFormat();

    bool open(const QString fileName,
              OstProto::FileType fileType,
              OstProto::FileMeta &meta,
              OstProto::FileContent &content,
              QString &error);
    bool save(OstProto::FileType fileType,
              const OstProto::FileContent &content,
              const QString fileName,
              QString &error);

    bool isNativeFileFormat(const QString fileName,
                            OstProto::FileType fileType);
    void postParseFixup(OstProto::FileMetaData metaData,
                        OstProto::FileContent &content);

private:
    void initFileMetaData(OstProto::FileMetaData &metaData);
    int fileMetaSize(const quint8* file, int size);

    static const int kFileMagicSize = 12;
    static const int kFileChecksumSize = 5;
    static const int kFileMinSize = kFileMagicSize + kFileChecksumSize;

    static const int kFileMagicOffset = 0;
    static const int kFileMetaDataOffset = kFileMagicSize;

    static const char* kFileMagicValue;

    // Native file format version
    static const uint kFileFormatVersionMajor = 0;
    static const uint kFileFormatVersionMinor = 2;
    static const uint kFileFormatVersionRevision = 4;
};

#endif
