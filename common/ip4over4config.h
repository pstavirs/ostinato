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

#ifndef _IP_4_OVER_4_CONFIG_H
#define _IP_4_OVER_4_CONFIG_H

#include "comboprotocolconfig.h"
#include "ip4config.h"
#include "ip4.h"

#include "protocol.pb.h"

typedef ComboProtocolConfigForm <
        OstProto::Protocol::kIp4over4FieldNumber, 
        Ip4ConfigForm, Ip4ConfigForm,
        Ip4Protocol, Ip4Protocol
    > Ip4over4ConfigForm;

#endif
