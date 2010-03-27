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

#include <QMutableLinkedListIterator>

class AbstractProtocol;
class ProtocolList;

class ProtocolListIterator 
{
private:
    QMutableLinkedListIterator<AbstractProtocol*> *_iter;

public:
    ProtocolListIterator(ProtocolList &list);
    ~ProtocolListIterator();
    bool findNext(const AbstractProtocol* value) const;
    bool findPrevious(const AbstractProtocol* value);
    bool hasNext() const;
    bool hasPrevious() const;
    void insert(AbstractProtocol* value);
    AbstractProtocol* next();
    AbstractProtocol* peekNext() const;
    AbstractProtocol* peekPrevious() const;
    AbstractProtocol* previous();
    void remove();
    void setValue(AbstractProtocol* value) const;
    void toBack();
    void toFront();
    const AbstractProtocol* value() const;
    AbstractProtocol* value();
};
