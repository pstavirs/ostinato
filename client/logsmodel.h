/*
Copyright (C) 2018 Srivats P.

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

#ifndef _LOGS_MODEL_H
#define _LOGS_MODEL_H

#include <QAbstractTableModel>
#include <QTime>

class LogsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    enum LogLevel { // FIXME: use enum class?
        kInfo,
        kWarning,
        kError,
        kLevelCount
    };

public:
    LogsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    int columnCount(const QModelIndex &parent=QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public slots:
    void clear();
    void setLogLevel(int level);
    void log(int logLevel,QString port, QString message);

private:
    struct Log {
        QTime timeStamp;
        int logLevel;
        QString port;
        QString message;
    };
    QVector<Log> logs_;
    LogLevel currentLevel_{kInfo};
};
#endif

