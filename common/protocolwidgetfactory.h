/*
Copyright (C) 2014 Srivats P.

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

#ifndef _PROTOCOL_WIDGET_FACTORY_H
#define _PROTOCOL_WIDGET_FACTORY_H

#include <QMap>

class AbstractProtocolConfigForm;

// Singleton class
class ProtocolWidgetFactory
{
    static QMap<int, void*>    configWidgetFactory;

public:
    ProtocolWidgetFactory();
    ~ProtocolWidgetFactory();

    // TODO: make registerProtocolConfigWidget static??
    // TODO: define a function pointer prototype instead of void* for
    //       protoConfigWidgetInstanceCreator
    static void registerProtocolConfigWidget(int protoNumber, 
            void *protoConfigWidgetInstanceCreator);

    AbstractProtocolConfigForm* createConfigWidget(int protoNumber);
    void deleteConfigWidget(AbstractProtocolConfigForm *configWidget);
};

#endif
