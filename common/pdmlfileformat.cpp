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

#include "pdmlfileformat.h"
#include "pdml_p.h"

PdmlFileFormat pdmlFileFormat;

PdmlFileFormat::PdmlFileFormat()
{
}

PdmlFileFormat::~PdmlFileFormat()
{
}

bool PdmlFileFormat::openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error)
{
    bool isOk = false;
    QFile file(fileName);
    PdmlReader *reader = new PdmlReader(&streams);

    if (!file.open(QIODevice::ReadOnly))
        goto _open_fail;

    connect(reader, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    emit status("Reading PDML packets...");
    emit target(100); // in percentage

    // TODO: fill in error string

    isOk = reader->read(&file); 

    goto _exit;

_open_fail:
    isOk = false;

_exit:
    delete reader;

    return isOk;
}

bool PdmlFileFormat::saveStreams(const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    error = "Save to PDML format is not supported";
    return false;
}

bool PdmlFileFormat::isMyFileFormat(const QString fileName)
{
    bool ret = false;
    QFile file(fileName);
    QByteArray buf;
    QXmlStreamReader xml;

    if (!file.open(QIODevice::ReadOnly))
        goto _exit;

    xml.setDevice(&file);

    xml.readNext();
    if (xml.hasError() || !xml.isStartDocument())
        goto _close_exit;

    xml.readNext();
    if (!xml.hasError() && xml.isStartElement() && (xml.name() == "pdml"))
        ret = true;
    else
        ret = false;

_close_exit:
    xml.clear();
    file.close();
_exit:
    return ret;
}

bool PdmlFileFormat::isMyFileType(const QString fileType)
{
    if (fileType.startsWith("PDML"))
        return true;
    else
        return false;
}
