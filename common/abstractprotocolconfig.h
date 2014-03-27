/*
Copyright (C) 2013-2014 Srivats P.

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
#ifndef _ABSTRACT_PROTOCOL_CONFIG_H
#define _ABSTRACT_PROTOCOL_CONFIG_H

#include <QWidget>

class AbstractProtocol;

class AbstractProtocolConfigForm : public QWidget
{
    Q_OBJECT
public:
/*! 
  Constructs the widget
*/
    AbstractProtocolConfigForm(QWidget *parent = 0)
        : QWidget(parent)
    {
        // Do nothing!
    }

/*! 
  Destroys the widget
*/
    virtual ~AbstractProtocolConfigForm() 
    {
        // Do nothing!
    }

/*! 
  Allocates and returns a new instance of the widget. 
  
  Caller is responsible for freeing up after use.  Subclasses MUST implement 
  this function 
*/
    static AbstractProtocolConfigForm* createInstance()
    {
        return NULL;
    }

/*!
  Loads data from the protocol using it's fieldData() method into this 
  widget

  Subclasses MUST implement this function. See the SampleProtocol for 
  an example
*/
    virtual void loadWidget(AbstractProtocol *proto)
    {
        // Do nothing!
    }

/*!
  Stores data from this widget into the protocol using the protocol's
  setFieldData() method

  Subclasses MUST implement this function. See the SampleProtocol for 
  an example
*/
    virtual void storeWidget(AbstractProtocol *proto)
    {
        // Do nothing!
    }

/*!
  Convenience Method - can be used by storeConfigWidget() implementations
*/
    uint hexStrToUInt(QString text, bool *ok=NULL)
    {
        bool isOk;
        uint a_uint =  text.remove(QChar(' ')).toUInt(&isOk, 16);

        if (ok)
            *ok = isOk;

        return a_uint;
    }
};

#endif
