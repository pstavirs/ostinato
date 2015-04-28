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

#ifndef _SHARED_PROTOBUF_MESSAGE_H
#define _SHARED_PROTOBUF_MESSAGE_H

#include <google/protobuf/message.h>
#include <QMutex>

// TODO: Use QSharedPointer instead once the minimum Qt version becomes >= 4.5

template <class T>
class SharedPointer
{
public:
    SharedPointer(T *ptr = 0)
        : ptr_(ptr)
    {
        mutex_ = new QMutex();
        refCnt_ = new unsigned int;
        *refCnt_ = 1;
        qDebug("sharedptr %p(constr) refcnt %p(%u)", this, refCnt_, *refCnt_);
    }

    ~SharedPointer()
    {
        mutex_->lock();

        (*refCnt_)--;
        if (*refCnt_ == 0) {
            delete ptr_;
            delete refCnt_;

            mutex_->unlock();
            delete mutex_;
            qDebug("sharedptr %p destroyed", this);
            return;
        }

        qDebug("sharedptr %p(destr) refcnt %p(%u)", this, refCnt_, *refCnt_);
        mutex_->unlock();
    }

    SharedPointer(const SharedPointer<T> &other)
    {
        ptr_ = other.ptr_;
        refCnt_ = other.refCnt_;
        mutex_ = other.mutex_;

        mutex_->lock();
        (*refCnt_)++;
        qDebug("sharedptr %p(copy) refcnt %p(%u)", this, refCnt_,*refCnt_);
        mutex_->unlock();
    }

    T* operator->() const
    {
        return ptr_;
    }

protected:
    T *ptr_;

    // use uint+mutex to simulate a QAtomicInt
    unsigned int *refCnt_;
    QMutex *mutex_;
};

typedef class SharedPointer< ::google::protobuf::Message> SharedProtobufMessage;

#endif

