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

#include "protocolwidgetfactory.h"

#include "macconfig.h"
#include "payloadconfig.h"
#include "vlanconfig.h"
#include "svlanconfig.h"
#include "vlanstackconfig.h"
#include "eth2config.h"
#include "dot3config.h"
#include "llcconfig.h"
#include "dot2llcconfig.h"
#include "snapconfig.h"
#include "dot2snapconfig.h"
#include "arpconfig.h"
#include "ip4config.h"
#include "ip6config.h"
#include "ip4over4config.h"
#include "ip4over6config.h"
#include "ip6over4config.h"
#include "ip6over6config.h"

ProtocolWidgetFactory *OstProtocolWidgetFactory;
QMap<int, void*> ProtocolWidgetFactory::configWidgetFactory;

ProtocolWidgetFactory::ProtocolWidgetFactory()
{
    /*! 
     * Ideally Protocol Widgets should register themselves
     *  with the Factory
     */
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kMacFieldNumber, 
            (void*) MacConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kPayloadFieldNumber, 
            (void*) PayloadConfigForm::createInstance);

    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kVlanFieldNumber, 
            (void*) VlanConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kSvlanFieldNumber, 
            (void*) SVlanConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kVlanStackFieldNumber, 
            (void*) VlanStackConfigForm::createInstance);

    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kEth2FieldNumber, 
            (void*) Eth2ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kDot3FieldNumber, 
            (void*) Dot3ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kLlcFieldNumber, 
            (void*) LlcConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kDot2LlcFieldNumber, 
            (void*) Dot2LlcConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kSnapFieldNumber, 
            (void*) SnapConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kDot2SnapFieldNumber, 
            (void*) Dot2SnapConfigForm::createInstance);

    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kArpFieldNumber, 
            (void*) ArpConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp4FieldNumber, 
            (void*) Ip4ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp6FieldNumber, 
            (void*) Ip6ConfigForm::createInstance);

    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp4over4FieldNumber, 
            (void*) Ip4over4ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp4over6FieldNumber, 
            (void*) Ip4over6ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp6over4FieldNumber, 
            (void*) Ip6over4ConfigForm::createInstance);
    OstProtocolWidgetFactory->registerProtocolConfigWidget(
            OstProto::Protocol::kIp6over6FieldNumber, 
            (void*) Ip6over6ConfigForm::createInstance);
}

ProtocolWidgetFactory::~ProtocolWidgetFactory()
{
    configWidgetFactory.clear();
}

void ProtocolWidgetFactory::registerProtocolConfigWidget(int protoNumber,
    void *protoConfigWidgetInstanceCreator)
{
    Q_ASSERT(!configWidgetFactory.contains(protoNumber));

    configWidgetFactory.insert(protoNumber, protoConfigWidgetInstanceCreator);
}

AbstractProtocolConfigForm* ProtocolWidgetFactory::createConfigWidget(
        int protoNumber)
{
    AbstractProtocolConfigForm* (*pc)();
    AbstractProtocolConfigForm* p;

    pc = (AbstractProtocolConfigForm* (*)()) 
            configWidgetFactory.value(protoNumber);
    
    Q_ASSERT_X(pc != NULL, 
               __FUNCTION__, 
               QString(protoNumber).toAscii().constData());

    p = (*pc)();

    return p;
}

void ProtocolWidgetFactory::deleteConfigWidget(
            AbstractProtocolConfigForm *configWidget)
{
    delete configWidget;
}
