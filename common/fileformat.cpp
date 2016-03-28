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

#include "fileformat.h"

FileFormat fileFormat;

FileFormat::FileFormat()
    : AbstractFileFormat(), NativeFileFormat()
{
    // Do Nothing!
}

bool FileFormat::openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error)
{
    OstProto::FileMeta meta;
    OstProto::FileContent content;
    bool ret = NativeFileFormat::open(fileName, meta, content, error);
    
    if (!ret)
        goto _fail;

    if (!content.matter().has_streams())
        goto _missing_streams;

    postParseFixup(meta.data(), content);

    streams.CopyFrom(content.matter().streams());

    return true;

_missing_streams:
    error = QString(tr("%1 does not contain any streams")).arg(fileName);
    goto _fail;
_fail:
    qDebug("%s", error.toAscii().constData());
    return false;
}

bool FileFormat::saveStreams(const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    OstProto::FileContent content;

    if (!streams.IsInitialized())
        goto _stream_not_init;

    content.mutable_matter()->mutable_streams()->CopyFrom(streams);
    Q_ASSERT(content.IsInitialized());

    return NativeFileFormat::save(OstProto::kStreamsFileType, content,
                                  fileName, error);

_stream_not_init:
    error = QString(tr("Internal Error: Streams not initialized\n%1\n%2"))
                .arg(QString().fromStdString(
                            streams.InitializationErrorString()))
                .arg(QString().fromStdString(streams.DebugString()));
    goto _fail;
_fail:
    qDebug("%s", error.toAscii().constData());
    return false;
}

bool FileFormat::isMyFileFormat(const QString fileName)
{
    return isNativeFileFormat(fileName, OstProto::kStreamsFileType);
}

bool FileFormat::isMyFileType(const QString fileType)
{
    if (fileType.startsWith("Ostinato"))
        return true;
    else
        return false;
}

