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

#ifndef _SERVER_PORT_MANAGER_H
#define _SERVER_PORT_MANAGER_H

#include <QList>
#include "abstractport.h"

class PortManager
{
public:
    PortManager();
    ~PortManager();

    int portCount() { return portList_.size(); }
    AbstractPort* port(int id) { return portList_[id]; }

    static PortManager* instance();

private:
    QList<AbstractPort*>    portList_;

    static PortManager      *instance_;
};

#endif
