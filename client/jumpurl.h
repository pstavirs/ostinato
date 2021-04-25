/*
Copyright (C) 2017 Srivats P.

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

#ifndef _JUMP_URL_H
#define _JUMP_URL_H

#include <QString>

inline QString jumpUrl(
        QString keyword,
        QString source="app",
        QString medium="hint",
        QString name="help")
{
    return QString("https://jump.ostinato.org/" + keyword + "?"
            + "utm_source=" + source + "&"
            + "utm_medium=" + medium + "&"
            + "utm_campaign=" + name);
}

#endif

