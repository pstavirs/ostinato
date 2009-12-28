#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>
#include <QString>
#include <QList>

#include "../common/protocol.pb.h"
#include "../common/streambase.h"

class Stream : public StreamBase {

    //quint32                    mId;

public:
    Stream();
    ~Stream();

    void loadProtocolWidgets();
    void storeProtocolWidgets();
};

#endif
