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

#ifndef _SESSION_FILE_FORMAT_H
#define _SESSION_FILE_FORMAT_H

#include "fileformat.pb.h"
#include "protocol.pb.h"

#include <QThread>
#include <QString>

class QDialog;

class SessionFileFormat : public QThread
{
    Q_OBJECT
public:
    enum Operation { kOpenFile, kSaveFile };

    SessionFileFormat();
    virtual ~SessionFileFormat();

    virtual bool open(const QString fileName,
            OstProto::SessionContent &session, QString &error) = 0;
    virtual bool save(const OstProto::SessionContent &session,
            const QString fileName, QString &error) = 0;

    virtual QDialog* openOptionsDialog();
    virtual QDialog* saveOptionsDialog();

    void openAsync(const QString fileName,
            OstProto::SessionContent &session, QString &error);
    void saveAsync(const OstProto::SessionContent &session,
            const QString fileName, QString &error);

    bool result();

    static QStringList supportedFileTypes(Operation op);

    static SessionFileFormat* fileFormatFromFile(const QString fileName);
    static SessionFileFormat* fileFormatFromType(const QString fileType);

    virtual bool isMyFileFormat(const QString fileName) = 0;
    virtual bool isMyFileType(const QString fileType) = 0;

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
    OstProto::SessionContent *openSession_;
    const OstProto::SessionContent *saveSession_;
    QString *error_;
    Operation op_;
    bool result_;

};

#endif

