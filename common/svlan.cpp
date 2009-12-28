#include <qendian.h>

#include "svlan.h"
#include "svlan.pb.h"

SVlanProtocol::SVlanProtocol(StreamBase *stream, AbstractProtocol *parent)
    : VlanProtocol(stream, parent)
{
    data.set_tpid(0x88a8);
    data.set_is_override_tpid(true);
}

SVlanProtocol::~SVlanProtocol()
{
}

AbstractProtocol* SVlanProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SVlanProtocol(stream, parent);
}

quint32 SVlanProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSvlanFieldNumber;
}

void SVlanProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::svlan)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SVlanProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::svlan))
        data.MergeFrom(protocol.GetExtension(OstProto::svlan));
}

QString SVlanProtocol::name() const
{
    return QString("SVlan");
}

QString SVlanProtocol::shortName() const
{
    return QString("SVlan");
}
