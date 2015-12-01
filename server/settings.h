/*
Copyright (C) 2014 Srivats P.

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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QSettings>
#include <QString>

extern QSettings *appSettings;

//
// General Section Keys
//
const QString kRateAccuracyKey("RateAccuracy");
const QString kRateAccuracyDefaultValue("High");

//
// RpcServer Section Keys
//
const QString kRpcServerAddress("RpcServer/Address");

//
// PortList Section Keys
//
const QString kPortListIncludeKey("PortList/Include");
const QString kPortListExcludeKey("PortList/Exclude");

#endif
