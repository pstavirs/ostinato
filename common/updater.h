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

#ifndef _UPDATER_H
#define _UPDATER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class Updater : public QObject
{
    Q_OBJECT
public:
    Updater();
    virtual ~Updater();
    void checkForNewVersion();
    static bool isVersionNewer(QString newVersion, QString curVersion);

signals:
    void newVersionAvailable(QString);
    void latestVersion(QString);

private slots:
    void parseXml(QNetworkReply *reply);

private:
    QString userAgent();
    QString sysInfo();

    QNetworkAccessManager *http_;
};

#endif

