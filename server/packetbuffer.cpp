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

#include "packetbuffer.h"

// PacketBuffer with full control
PacketBuffer::PacketBuffer(int size)
{
    if (size == 0)
        size = 1600;

    buffer_ = new uchar[size];
    is_own_buffer_ = true;

    head_ = data_ = tail_ = buffer_;
    end_ = head_ + size;
}

// PacketBuffer wrapping already existing const buffer
PacketBuffer::PacketBuffer(const uchar *buffer, int size)
{
    // FIXME: ugly const_cast hack!!
    buffer_ = const_cast<uchar*>(buffer);
    is_own_buffer_ = false;

    head_ = data_ = buffer_;
    tail_ = end_ = buffer_ + size;
}

PacketBuffer::~PacketBuffer()
{
    if (is_own_buffer_)
        delete[] buffer_;
}

int PacketBuffer::length() const
{
    return tail_ - data_;
}

uchar* PacketBuffer::head() const
{
    return head_;
}

uchar* PacketBuffer::data() const
{
    return data_;
}

uchar* PacketBuffer::tail() const
{
    return tail_;
}

uchar* PacketBuffer::end() const
{
    return end_;
}

void PacketBuffer::reserve(int len)
{
    data_ += len;
    tail_ += len;
}

uchar* PacketBuffer::pull(int len)
{
    if ((tail_ - data_) < len)
        return NULL;
    data_ += len;

    return data_;
}

uchar* PacketBuffer::push(int len)
{
    if ((data_ - head_) < len)
        return NULL;
    data_ -= len;

    return data_;
}

uchar* PacketBuffer::put(int len)
{
    uchar *oldTail = tail_;

    if ((end_ - tail_) < len)
        return NULL;
    tail_ += len;

    return oldTail;
}
