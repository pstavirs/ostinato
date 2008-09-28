#include <vector>

#include <google/protobuf/descriptor.h>

#include "port.h"
#include "pbhelper.h"

uint Port::mAllocStreamId = 0;

uint Port::newStreamId()
{
	return mAllocStreamId++;
}

Port::Port(quint32 id, quint32 portGroupId)
{
	mPortId = id;
	d.mutable_port_id()->set_id(id);
	stats.mutable_port_id()->set_id(id);
	mPortGroupId = portGroupId;
}

Port::~Port()
{
}

void Port::updatePortConfig(OstProto::Port *port)
{
	d.MergeFrom(*port);
}

void Port::updateStreamOrdinalsFromIndex()
{
	for (int i=0; i < mStreams.size(); i++)
		mStreams[i].setOrdinal(i);
}

void Port::reorderStreamsByOrdinals()
{
	qSort(mStreams);
}

bool Port::newStreamAt(int index)
{
	Stream	s;

	if (index > mStreams.size())
		return false;

	s.setId(newStreamId());
	mStreams.insert(index, s);
	updateStreamOrdinalsFromIndex();

	return true;
}

bool Port::deleteStreamAt(int index)
{
	if (index >= mStreams.size())
		return false;

	mStreams.removeAt(index);
	updateStreamOrdinalsFromIndex();

	return true;
}

bool Port::insertStream(uint streamId)
{
	Stream	s;

	s.setId(streamId);

	// FIXME(MED): If a stream with id already exists, what do we do?
	mStreams.append(s);

	// Update mAllocStreamId to take into account the stream id received
	// from server
	if (mAllocStreamId <= streamId)
		mAllocStreamId = streamId + 1;

	return true;
}

bool Port::updateStream(uint streamId, OstProto::Stream *stream)
{
	int i, streamIndex;

	for (i = 0; i < mStreams.size(); i++)
	{
		if (streamId == mStreams[i].id())
			goto _found;
	}

	qDebug("%s: Invalid stream id %d", __FUNCTION__, streamId);
	return false;

_found:
	streamIndex = i;

	mStreams[streamIndex].update(stream);
	reorderStreamsByOrdinals();

	return true;
}

void Port::getDeletedStreamsSinceLastSync(
	OstProto::StreamIdList &streamIdList)
{
	streamIdList.clear_stream_id();
	for (int i = 0; i < mLastSyncStreamList.size(); i++)
	{
		int j;

		for (j = 0; j < mStreams.size(); j++)
		{
			if (mLastSyncStreamList[i] == mStreams[j].id())
				break;
		}

		if (j < mStreams.size())
		{
			// stream still exists!
			continue;
		}	
		else
		{
			// stream has been deleted since last sync
			OstProto::StreamId	*s;

			s = streamIdList.add_stream_id();
			s->set_id(mLastSyncStreamList.at(i));
		}
	}
}

void Port::getNewStreamsSinceLastSync(
	OstProto::StreamIdList &streamIdList)
{
	streamIdList.clear_stream_id();
	for (int i = 0; i < mStreams.size(); i++)
	{
		if (mLastSyncStreamList.contains(mStreams[i].id()))
		{
			// existing stream!
			continue;
		}
		else
		{
			// new stream!
			OstProto::StreamId	*s;

			s = streamIdList.add_stream_id();
			s->set_id(mStreams[i].id());
		}
	}
}

void Port::getModifiedStreamsSinceLastSync(
	OstProto::StreamConfigList &streamConfigList)
{
	qDebug("In %s", __FUNCTION__);

	//streamConfigList.mutable_port_id()->set_id(mPortId);
	for (int i = 0; i < mStreams.size(); i++)
	{
		OstProto::Stream	*s;

		s = streamConfigList.add_stream();
		mStreams[i].getConfig(mPortId, s);
	}
}

void Port::when_syncComplete()
{
	qSort(mStreams);

	mLastSyncStreamList.clear();
	for (int i=0; i<mStreams.size(); i++)
		mLastSyncStreamList.append(mStreams[i].id());
}

void Port::updateStats(OstProto::PortStats *portStats)
{
	stats.MergeFrom(*portStats);
}

