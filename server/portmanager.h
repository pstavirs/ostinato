#ifndef _SERVER_PORT_MANAGER_H
#define _SERVER_PORT_MANAGER_H

#include <QList>
#include "abstractport.h"

class PortManager
{
public:
    PortManager();
    ~PortManager();

    int portCount() { return portList_.size(); }
    AbstractPort* port(int id) { return portList_[id]; }

    static PortManager* instance();

private:
    QList<AbstractPort*>    portList_;

    static PortManager      *instance_;
};

#endif
