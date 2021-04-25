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

#ifndef _PARAMS_H
#define _PARAMS_H

#include <QStringList>

class Params {
public:
    Params();
    int parseCommandLine(int argc, char* argv[]);

    bool optLocalDrone();
    bool optLogsDisabled();

    int argumentCount();
    QString argument(int index);

private:
    bool localDrone_;
    bool logsDisabled_;
    QStringList args_;
};

extern Params appParams;

#endif

