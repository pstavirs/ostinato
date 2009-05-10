#ifndef _STREAM_BASE_H
#define _STREAM_BASE_H

#include <QList>

#include "protocolcollection.h"

class StreamBase
{
protected: // TODO: temp - make private
	OstProto::StreamId	 	*mStreamId;
	OstProto::StreamCore 	*mCore;
	OstProto::StreamControl	*mControl;

private:
	ProtocolList			currentFrameProtocols;
protected:
	ProtocolCollection		protocols;

public:
	StreamBase();
	~StreamBase();

	void protoDataCopyFrom(const OstProto::Stream &stream);
	void protoDataCopyInto(OstProto::Stream &stream) const;

	QList<int> frameProtocol();
	void setFrameProtocol(QList<int> protocolList);

	AbstractProtocol* protocol(int protoNum);
	AbstractProtocol* protocol(QString protoName);

	// TODO: make a copy constructor
};

#endif
