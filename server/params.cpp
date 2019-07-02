/*
Copyright (C) 2019 Srivats P.

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

extern char *version;
extern char *revision;

Params::Params()
{
    logsDisabled_ = true;
    myPort_ = 7878;
}

int Params::parseCommandLine(int argc, char* argv[])
{
    int c, n = 0;

    opterr = 0;
    while ((c = getopt (argc, argv, "dhp:v")) != -1) {
        switch (c)
        {
        case 'd':
            logsDisabled_ = false;
            break;
        case 'p':
            myPort_ = atoi(optarg);
            break;
        case 'v':
            printf("Ostinato Drone %s rev %s\n", version, revision);
            exit(0);
        case 'h':
        default:
            printf("usage: %s [-dhv] [-p <port-number>]\n", argv[0]);
            exit(1);
        }
        n++;
    }

    for (int i = optind; i < argc; i++, n++)
        args_ << argv[i];

    return n;
}

bool Params::optLogsDisabled()
{
    return logsDisabled_;
}

int Params::servicePortNumber()
{
    return myPort_;
}

int Params::argumentCount()
{
    return args_.size();
}

QString Params::argument(int index)
{
    return index < args_.size() ? args_.at(index) : QString();
}
