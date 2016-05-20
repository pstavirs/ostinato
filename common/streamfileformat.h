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

#ifndef _STREAM_FILE_FORMAT_H
#define _STREAM_FILE_FORMAT_H

#include "protocol.pb.h"

#include <QThread>
#include <QString>

class QDialog;

class StreamFileFormat : public QThread
{
    Q_OBJECT
public:
    enum Operation { kOpenFile, kSaveFile };

    StreamFileFormat();
    virtual ~StreamFileFormat();

    virtual bool open(const QString fileName,
            OstProto::StreamConfigList &streams, QString &error) = 0;
    virtual bool save(const OstProto::StreamConfigList streams,
            const QString fileName, QString &error) = 0;

    virtual QDialog* openOptionsDialog();
    virtual QDialog* saveOptionsDialog();

    void openAsync(const QString fileName,
            OstProto::StreamConfigList &streams, QString &error);
    void saveAsync(const OstProto::StreamConfigList streams,
            const QString fileName, QString &error);

    bool result();

    static QStringList supportedFileTypes(Operation op);

    static StreamFileFormat* fileFormatFromFile(const QString fileName);
    static StreamFileFormat* fileFormatFromType(const QString fileType);

#if 0
    bool isMyFileFormat(const QString fileName) = 0;
    bool isMyFileType(const QString fileType) = 0;
#endif

signals:
    void status(QString text);
    void target(int value);
    void progress(int value);

public slots:
    void cancel();

protected:
    void run();

    bool stop_;

private:
    QString fileName_;
    OstProto::StreamConfigList *openStreams_;
    OstProto::StreamConfigList saveStreams_;
    QString *error_;
    Operation op_;
    bool result_;

};

#endif

