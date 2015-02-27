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

#ifndef _COMBO_PROTOCOL_CONFIG_H
#define _COMBO_PROTOCOL_CONFIG_H

#include "abstractprotocolconfig.h"
#include "comboprotocol.h"

template <int protoNumber, 
         class FormA, class FormB,
         class ProtoA, class ProtoB>
class ComboProtocolConfigForm : 
    public AbstractProtocolConfigForm
{
public:
    ComboProtocolConfigForm(QWidget *parent = 0)
        : AbstractProtocolConfigForm(parent)
    {
        QVBoxLayout    *layout = new QVBoxLayout;

        formA = new FormA(this);
        formB = new FormB(this);

        layout->addWidget(formA);
        layout->addWidget(formB);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        qDebug("%s: protoNumber = %d, %p <--> %p", __FUNCTION__,
            protoNumber, formA, formB);
    }

    virtual ~ComboProtocolConfigForm()
    {
        formA->setParent(0);
        formB->setParent(0);

        delete formA;
        delete formB;
    }

    static ComboProtocolConfigForm* createInstance()
    {
        return new ComboProtocolConfigForm<protoNumber, 
               FormA, FormB, ProtoA, ProtoB>;
    }

    virtual void loadWidget(AbstractProtocol *proto)
    {
        class ComboProtocol<protoNumber, ProtoA, ProtoB> *comboProto = 
            dynamic_cast<ComboProtocol<protoNumber, ProtoA, ProtoB>*>(proto);

        Q_ASSERT_X(comboProto != NULL, 
                QString("ComboProtocolConfigForm{%1}::loadWidget()")
                    .arg(protoNumber).toAscii().constData(),
                QString("Protocol{%1} is not a instance of ComboProtocol")
                    .arg(proto->protocolNumber()).toAscii().constData());

        formA->loadWidget(comboProto->protoA);
        formB->loadWidget(comboProto->protoB);
    }
    virtual void storeWidget(AbstractProtocol *proto)
    {
        class ComboProtocol<protoNumber, ProtoA, ProtoB> *comboProto = 
            dynamic_cast<ComboProtocol<protoNumber, ProtoA, ProtoB>*>(proto);

        Q_ASSERT_X(comboProto != NULL, 
                QString("ComboProtocolConfigForm{%1}::loadWidget()")
                    .arg(protoNumber).toAscii().constData(),
                QString("Protocol{%1} is not a instance of ComboProtocol")
                    .arg(proto->protocolNumber()).toAscii().constData());

        formA->storeWidget(comboProto->protoA);
        formB->storeWidget(comboProto->protoB);
    }

protected:
    FormA    *formA;
    FormB    *formB;
};

#endif
