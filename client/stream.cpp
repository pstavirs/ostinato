#include <qendian.h>
#include <QHostAddress>

#include "stream.h"
//#include "../common/protocollist.h"
#include "../common/protocollistiterator.h"
#include "../common/abstractprotocol.h"

Stream::Stream()
{
    //mId = 0xFFFFFFFF;
    setEnabled(true);
}

Stream::~Stream()
{
}

void Stream::loadProtocolWidgets()
{
#if 0
    //protocols.loadConfigWidgets();
    foreach(AbstractProtocol* proto, *currentFrameProtocols)
    {
        proto->loadConfigWidget();
    }
#else
    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol* p = iter->next();
        p->loadConfigWidget();
    }
    delete iter;
#endif
}

void Stream::storeProtocolWidgets()
{
#if 0
    //protocols.storeConfigWidgets();
    foreach(const AbstractProtocol* proto, frameProtocol())
    {
        proto->storeConfigWidget();
        _iter->toFront();
    }
#else
    ProtocolListIterator    *iter;

    iter = createProtocolListIterator();
    while (iter->hasNext())
    {
        AbstractProtocol* p = iter->next();
        p->storeConfigWidget();
    }
    delete iter;
#endif
}
