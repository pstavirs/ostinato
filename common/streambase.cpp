#include "streambase.h"

StreamBase::StreamBase() :
	mStreamId(new OstProto::StreamId),
	mCore(new OstProto::StreamCore),
	mControl(new OstProto::StreamControl),
	protocols(currentFrameProtocols, mCore)
{
	mStreamId->set_id(0xFFFFFFFF);
}

StreamBase::~StreamBase()
{
	delete mStreamId;
	delete mCore;
	delete mControl;
}

void StreamBase::protoDataCopyFrom(const OstProto::Stream &stream)
{
	mStreamId->CopyFrom(stream.stream_id());
	mCore->CopyFrom(stream.core());
	mControl->CopyFrom(stream.control());

	protocols.protoDataCopyFrom(stream);
	setFrameProtocol(frameProtocol());
}

void StreamBase::protoDataCopyInto(OstProto::Stream &stream) const
{
	stream.mutable_stream_id()->CopyFrom(*mStreamId);
	stream.mutable_core()->CopyFrom(*mCore);
	stream.mutable_control()->CopyFrom(*mControl);

	protocols.protoDataCopyInto(stream);
}

QList<int> StreamBase::frameProtocol()
{
	QList<int> protocolList;

	for (int i = 0; i < mCore->frame_proto_size(); i++)
		protocolList.append(mCore->frame_proto(i));

	return protocolList;
}

void StreamBase::setFrameProtocol(QList<int> protocolList)
{
	mCore->clear_frame_proto();
	currentFrameProtocols.clear();

	for (int i = 0; i < protocolList.size(); i++)
	{
		mCore->add_frame_proto(protocolList.at(i));
		currentFrameProtocols.append(protocols.protocol(protocolList.at(i)));
	}
}

AbstractProtocol* StreamBase::protocol(int protoNum)
{
	return protocols.protocol(protoNum);
}

AbstractProtocol* StreamBase::protocol(QString protoName)
{
	return protocols.protocol(protoName);
}

