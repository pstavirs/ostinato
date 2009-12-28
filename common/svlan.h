#ifndef _SVLAN_H
#define _SVLAN_H

#include "vlan.h"

class SVlanProtocol : public VlanProtocol
{
public:
    SVlanProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~SVlanProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;
};

#endif
