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

#ifndef _PB_RPC_COMMON_H
#define _PB_RPC_COMMON_H

// Print a HexDump
#define BUFDUMP(ptr, len) qDebug("%s", QString(QByteArray((char*)(ptr), \
    (len)).toHex()).toAscii().data()); 

/*
** RPC Header (8)
**    - MSG_TYPE (2)
**    - METHOD_ID (2)
**    - LEN (4) [not including this header]
*/
#define PB_HDR_SIZE                8

#define PB_MSG_TYPE_REQUEST        1
#define PB_MSG_TYPE_RESPONSE    2
#define PB_MSG_TYPE_BINBLOB        3
#define PB_MSG_TYPE_ERROR          4

#endif
