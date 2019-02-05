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

#ifndef _LOG_H
#define _LOG_H

#include "logsmodel.h"

extern LogsModel *appLogs;

inline static void logError(QString port, QString message)
{
    appLogs->log(LogsModel::kError, port, message);
}

inline static void logError(quint32 portGroupId, quint32 portId,QString message)
{
    logError(QString("%1-%2").arg(portGroupId).arg(portId), message);
}

inline static void logError(quint32 portGroupId, QString message)
{
    logError(QString("%1-*").arg(portGroupId), message);
}

inline static void logError(QString message)
{
    logError(QString("--"), message);
}

inline static void logWarn(QString port, QString message)
{
    appLogs->log(LogsModel::kWarning, port, message);
}

inline static void logWarn(quint32 portGroupId, quint32 portId,QString message)
{
    logWarn(QString("%1-%2").arg(portGroupId).arg(portId), message);
}

inline static void logWarn(quint32 portGroupId, QString message)
{
    logWarn(QString("%1-*").arg(portGroupId), message);
}

inline static void logWarn(QString message)
{
    logWarn(QString("--"), message);
}

inline static void logInfo(QString port, QString message)
{
    appLogs->log(LogsModel::kInfo, port, message);
}

inline static void logInfo(quint32 portGroupId, quint32 portId,QString message)
{
    logInfo(QString("%1-%2").arg(portGroupId).arg(portId), message);
}

inline static void logInfo(quint32 portGroupId, QString message)
{
    logInfo(QString("%1-*").arg(portGroupId), message);
}

inline static void logInfo(QString message)
{
    logInfo(QString("--"), message);
}
#endif

