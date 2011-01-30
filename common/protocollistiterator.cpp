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

#include "protocollistiterator.h"
#include "protocollist.h"
#include "abstractprotocol.h"

ProtocolListIterator::ProtocolListIterator(ProtocolList &list)
{
    _iter = new QMutableLinkedListIterator<AbstractProtocol*>(list);
}

ProtocolListIterator::~ProtocolListIterator()
{
    delete _iter;
}

bool ProtocolListIterator::findNext(const AbstractProtocol* value) const
{
    return _iter->findNext(const_cast<AbstractProtocol*>(value));
}

bool ProtocolListIterator::findPrevious(const AbstractProtocol* value)
{
    return _iter->findPrevious(const_cast<AbstractProtocol*>(value));
}

bool ProtocolListIterator::hasNext() const
{
    return _iter->hasNext();
}

bool ProtocolListIterator::hasPrevious() const
{
    return _iter->hasPrevious();
}

void ProtocolListIterator::insert(AbstractProtocol* value)
{
    if (_iter->hasPrevious())
    {
        value->prev = _iter->peekPrevious();
        value->prev->next = value;
    }
    else
        value->prev = NULL;

    if (_iter->hasNext())
    {
        value->next = _iter->peekNext();
        value->next->prev = value;
    }
    else
        value->next = NULL;

    _iter->insert(const_cast<AbstractProtocol*>(value));
}

AbstractProtocol* ProtocolListIterator::next()
{
    return _iter->next();
}

AbstractProtocol* ProtocolListIterator::peekNext() const
{
    return _iter->peekNext();
}

AbstractProtocol* ProtocolListIterator::peekPrevious() const
{
    return _iter->peekPrevious();
}

AbstractProtocol* ProtocolListIterator::previous()
{
    return _iter->previous();
}

void ProtocolListIterator::remove()
{
    if (_iter->value()->prev)
        _iter->value()->prev->next = _iter->value()->next;
    if (_iter->value()->next)
        _iter->value()->next->prev = _iter->value()->prev;
    _iter->remove();
}

void ProtocolListIterator::setValue(AbstractProtocol* value) const
{
    if (_iter->value()->prev)
        _iter->value()->prev->next = value;
    if (_iter->value()->next)
        _iter->value()->next->prev = value;
    value->prev = _iter->value()->prev;
    value->next = _iter->value()->next;
    _iter->setValue(const_cast<AbstractProtocol*>(value));
}

void ProtocolListIterator::toBack()
{
    _iter->toBack();
}

void ProtocolListIterator::toFront()
{
    _iter->toFront();
}

const AbstractProtocol* ProtocolListIterator::value() const
{
    return _iter->value();
}

AbstractProtocol* ProtocolListIterator::value()
{
    return _iter->value();
}
