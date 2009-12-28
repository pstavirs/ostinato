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
    return _iter->findNext((AbstractProtocol*)((uint)value));
}

bool ProtocolListIterator::findPrevious(const AbstractProtocol* value)
{
    return _iter->findPrevious((AbstractProtocol*)((uint)value));
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

    _iter->insert((AbstractProtocol*)((uint)value));
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
    _iter->setValue((AbstractProtocol*)((uint)value));
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
