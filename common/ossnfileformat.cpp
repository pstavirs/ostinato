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

#include "ossnfileformat.h"

OssnFileFormat ossnFileFormat;

OssnFileFormat::OssnFileFormat()
    : SessionFileFormat(), NativeFileFormat()
{
    // Do Nothing
}

bool OssnFileFormat::open(const QString fileName,
        OstProto::SessionContent &session, QString &error)
{
    OstProto::FileMeta meta;
    OstProto::FileContent content;
    bool ret = NativeFileFormat::open(fileName, OstProto::kSessionFileType,
                                      meta, content, error);
    if (!ret)
        goto _exit;

    if (!content.matter().has_session())
        goto _missing_session;

    postParseFixup(meta.data(), content);

    session.CopyFrom(content.matter().session());

    return true;

_missing_session:
    error = QString(tr("%1 does not contain a session")).arg(fileName);
    goto _fail;
_fail:
    qDebug("%s", qPrintable(error));
_exit:
    return false;
}

bool OssnFileFormat::save(const OstProto::SessionContent &session,
        const QString fileName, QString &error)
{
    OstProto::FileContent content;

    if (!session.IsInitialized())
        goto _session_not_init;

    content.mutable_matter()->mutable_session()->CopyFrom(session);
    Q_ASSERT(content.IsInitialized());

    return NativeFileFormat::save(OstProto::kSessionFileType, content,
                                  fileName, error);

_session_not_init:
    error = QString(tr("Internal Error: Session not initialized\n%1\n%2"))
                .arg(QString().fromStdString(
                            session.InitializationErrorString()))
                .arg(QString().fromStdString(session.DebugString()));
    goto _fail;
_fail:
    qDebug("%s", qPrintable(error));
    return false;
}

bool OssnFileFormat::isMyFileFormat(const QString fileName)
{
    return isNativeFileFormat(fileName, OstProto::kSessionFileType);
}

bool OssnFileFormat::isMyFileType(const QString fileType)
{
    return fileType.contains("(*.ossn)") ? true : false;
}
