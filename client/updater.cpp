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

#include "updater.h"

#include <QHttp>
#include <QTemporaryFile>
#include <QXmlStreamReader>

extern const char* version;

Updater::Updater()
{
    http_ = NULL;
    file_ = NULL;

#if 1
    // Tests!
    Q_ASSERT(isVersionNewer("1.1", "1") == true);
    Q_ASSERT(isVersionNewer("10.1", "2") == true);
    Q_ASSERT(isVersionNewer("0.10", "0.2") == true);
    Q_ASSERT(isVersionNewer("1.10.1", "1.2.3") == true);
#endif
}

Updater::~Updater()
{
    delete http_;
    delete file_;
}

void Updater::checkForNewVersion()
{
    http_ = new QHttp("wiki.ostinato.googlecode.com");
    file_ = new QTemporaryFile();

    connect(http_, SIGNAL(responseHeaderReceived(QHttpResponseHeader)), 
            this, SLOT(responseReceived(QHttpResponseHeader)));
    connect(http_, SIGNAL(requestFinished(int, bool)), 
            this, SLOT(parseXml(int, bool)));
    connect(http_, SIGNAL(stateChanged(int)), 
            this, SLOT(stateUpdate(int)));

    file_->open();
    qDebug("Updater: PAD XML file - %s", qPrintable(file_->fileName()));

    http_->get("/hg/html/pad.xml", file_);
    qDebug("Updater: %s", qPrintable(http_->currentRequest().toString()
                .replace("\r\n", "\nUpdater: ")));
}

void Updater::stateUpdate(int state)
{
    qDebug("Updater: state %d", state);
}

void Updater::responseReceived(QHttpResponseHeader response)
{
    qDebug("Updater: HTTP/%d.%d %d %s", 
            response.majorVersion(), response.minorVersion(),
            response.statusCode(), qPrintable(response.reasonPhrase()));
}

void Updater::parseXml(int id, bool error) 
{
    QXmlStreamReader xml;
    QString newVersion;

    if (error) {
        qDebug("Updater: %s", qPrintable(http_->errorString()));
        goto _exit;
    }

    // Close and reopen the file so that we read from the top
    file_->close();
    file_->open();
    xml.setDevice(file_);

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && (xml.name() == "Program_Version"))
            newVersion = xml.readElementText();
    }

    qDebug("Updater: latest version = %s", qPrintable(newVersion));
    if (!newVersion.isEmpty() && isVersionNewer(newVersion, QString(version)))
        emit newVersionAvailable(newVersion);

_exit:
    // Job done, time to self-destruct
    deleteLater();
}

bool Updater::isVersionNewer(QString newVersion, QString curVersion)
{
    QStringList curVer = QString(curVersion).split('.');
    QStringList newVer = QString(newVersion).split('.');

    for (int i = 0; i < qMin(curVer.size(), newVer.size()); i++) {
        bool isOk;
        if (newVer.at(i).toUInt(&isOk) > curVer.at(i).toUInt(&isOk))
            return true;
    }
    
    if (newVer.size() > curVer.size())
        return true;

    return false;
}


