/*
Copyright (C) 2015 Srivats P.

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

#ifndef _PYTHON_FILE_FORMAT_H
#define _PYTHON_FILE_FORMAT_H

#include "streamfileformat.h"

#include <QTextStream>

class PythonFileFormat : public StreamFileFormat
{
public:
    PythonFileFormat();
    ~PythonFileFormat();

    virtual bool open(const QString fileName,
            OstProto::StreamConfigList &streams, QString &error);
    virtual bool save(const OstProto::StreamConfigList streams,
            const QString fileName, QString &error);

    bool isMyFileFormat(const QString fileName);
    bool isMyFileType(const QString fileType);

private:
    void writeStandardImports(QTextStream &out);
    void writePrologue(QTextStream &out);
    void writeEpilogue(QTextStream &out);
    void writeFieldAssignment(QTextStream &out, 
            QString fieldName,
            const google::protobuf::Message &msg,
            const google::protobuf::Reflection *refl,
            const google::protobuf::FieldDescriptor *fieldDesc,
            int index = -1);
    QString singularize(QString plural);
    QString escapeString(QString str);
    bool useDecimalBase(QString fieldName);
};

extern PythonFileFormat pythonFileFormat;

#endif

