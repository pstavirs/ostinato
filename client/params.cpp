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

#include "params.h"

#include <unistd.h>

Params::Params()
{
    localDrone_ = true;
}

int Params::parseCommandLine(int argc, char* argv[])
{
    int c, n = 0;

    opterr = 0;
    while ((c = getopt (argc, argv, "s")) != -1) {
        switch (c)
        {
        case 's':
            localDrone_ = false;
            break;
        default:
            qDebug("ignoring unrecognized option (%c)", c);
        }
        n++;
    }

    for (int i = optind; i < argc; i++, n++)
        args_ << argv[i];

    return n;
}

bool Params::optLocalDrone()
{
    return localDrone_;
}

int Params::argumentCount()
{
    return args_.size();
}

QString Params::argument(int index)
{
    return index < args_.size() ? args_.at(index) : QString();
}
