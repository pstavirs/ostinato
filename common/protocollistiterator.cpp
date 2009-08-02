#include "protocollistiterator.h"
#include "protocollist.h"

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

void ProtocolListIterator::insert(const AbstractProtocol* value)
{
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
	_iter->remove();
}

void ProtocolListIterator::setValue(const AbstractProtocol* value) const
{
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
