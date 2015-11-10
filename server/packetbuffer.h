/*
Copyright (C) 2015 Srivats P.

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

#ifndef _PACKET_BUFFER_H
#define _PACKET_BUFFER_H

#include <QtGlobal>

class PacketBuffer
{
public:
    PacketBuffer(int size = 0);
    PacketBuffer(const uchar *buffer, int size);
    ~PacketBuffer();

    int length() const;

    uchar* head() const;
    uchar* data() const;
    uchar* tail() const;
    uchar* end() const;

    void reserve(int len);
    uchar* pull(int len);
    uchar* push(int len);
    uchar* put(int len);

private:
    uchar *buffer_;
    bool is_own_buffer_;
    uchar *head_, *data_, *tail_, *end_;
};

#endif

