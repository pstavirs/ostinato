#include <qendian.h>
#include <QHostAddress>

#include "stream.h"

#include "../common/mac.h"
#include "../common/payload.h"

#include "../common/eth2.h"	// FIXME: proto DB
#include "../common/dot3.h"	// FIXME: proto DB
#include "../common/llc.h"	// FIXME: proto DB
#include "../common/snap.h"	// FIXME: proto DB
#include "../common/ip4.h"	// FIXME: proto DB
#include "../common/tcp.h"	// FIXME: proto DB
#include "../common/udp.h"	// FIXME: proto DB


//-----------------------------------------------------
// Stream Class Methods
//-----------------------------------------------------
Stream::Stream()
{
	mId = 0xFFFFFFFF;

	mCore = new OstProto::StreamCore;
	mControl = new OstProto::StreamControl;

//	mCore->set_port_id(0xFFFFFFFF);
//	mCore->set_stream_id(mId);

	mProtocolList.append(new MacProtocol);
	mProtocolList.append(new PayloadProtocol(this));

	// FIXME: proto DB
	mProtocolList.append(new Eth2Protocol);
	mProtocolList.append(new Dot3Protocol);
	mProtocolList.append(new LlcProtocol);
	mProtocolList.append(new SnapProtocol);
	mProtocolList.append(new Ip4Protocol);
	mProtocolList.append(new TcpProtocol);
	mProtocolList.append(new UdpProtocol);

	mCore->set_is_enabled(true);
	mCore->add_frame_proto(51); // MAC (FIXME: hardcoding)
	mCore->add_frame_proto(52); // Payload (FIXME: hardcoding)
}

Stream::~Stream()
{
	for (int i = 0; i < mProtocolList.size(); i++)
		delete mProtocolList.at(i);

	delete mControl;
	delete mCore;
}

void Stream::protoDataCopyFrom(Stream& stream)
{
	OstProto::Stream  data;

	stream.getConfig(0, data);
	update(&data);
}

void Stream::loadProtocolWidgets()
{
	for (int i=0; i < mProtocolList.size(); i++)
		mProtocolList[i]->loadConfigWidget();
}

void Stream::storeProtocolWidgets()
{
	for (int i=0; i < mProtocolList.size(); i++)
		mProtocolList[i]->storeConfigWidget();
}

/*! Copy current client side config into the OstProto::Stream */
// FIXME - remove portId unused param!
void Stream::getConfig(uint portId, OstProto::Stream &s)
{
	s.mutable_stream_id()->set_id(mId);

	s.mutable_core()->CopyFrom(*mCore);
	s.mutable_control()->CopyFrom(*mControl);

	// FIXME - this doesn't take care of multiple headers of same proto
	// e.g. IPinIP or double VLAN Tagged
	// FIXME: change s from pointer to reference?
	for (int i = 0; i < mProtocolList.size(); i++)
	{
		qDebug("%s: protocol %d", __FUNCTION__, i);
		mProtocolList[i]->protoDataCopyInto(s);
	}

	qDebug("%s: Done", __FUNCTION__);
}

bool Stream::update(OstProto::Stream	*stream)
{
	mCore->clear_frame_proto();
	mCore->MergeFrom(stream->core());
	mControl->MergeFrom(stream->control());

	// FIXME - this doesn't take care of multiple headers of same proto
	// e.g. IPinIP or double VLAN Tagged
	// FIXME: change s from pointer to reference?
	for (int i = 0; i < mProtocolList.size(); i++)
	{
		mProtocolList[i]->protoDataCopyFrom(*stream);
	}

	// FIXME(MED): Re-eval why not store complete OstProto::Stream
	// instead of components
	return true;
}

QList<int> Stream::frameProtocol()
{
	QList<int> protocolList;

	for (int i = 0; i < mCore->frame_proto_size(); i++)
		protocolList.append(mCore->frame_proto(i));

	return protocolList;
}

void Stream::setFrameProtocol(QList<int> protocolList)
{
	mCore->clear_frame_proto();
	for (int i = 0; i < protocolList.size(); i++)
		mCore->add_frame_proto(protocolList.at(i));
}

int Stream::protocolHeaderSize()
{
	int size = 0;

	for (int i = 0; i < mCore->frame_proto_size(); i++)
		size += protocolById(mCore->frame_proto(i))->
			protocolFrameValue().size();

	return size;
}

AbstractProtocol* Stream::protocolById(int id)
{
	// FIXME BAD BAD VERY BAD!
	switch(id) {
		case 51:
			return mProtocolList.at(0);
		case 52:
			return mProtocolList.at(1);
		case 121:
			return mProtocolList.at(2);
		case 122:
			return mProtocolList.at(3);
		case 123:
			return mProtocolList.at(4);
		case 124:
			return mProtocolList.at(5);
			// case 125 (unused)
#if 0 // todo VLAN
		case 126:
			return mProtocolList.at(x);
#endif
		case 130:
			return mProtocolList.at(6);
		case 140:
			return mProtocolList.at(7);
		case 141:
			return mProtocolList.at(8);
		default:
			return NULL;
	}
}
