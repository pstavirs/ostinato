/*
Copyright (C) 2010 Srivats P.

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

#include "port.h"

#include "abstractfileformat.h"

#include <QApplication>
#include <QVariant>
#include <google/protobuf/descriptor.h>
#include <vector>

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
    capFile_ = NULL;
}

Port::~Port()
{
    qDebug("%s", __FUNCTION__);
    while (!mStreams.isEmpty())
        delete mStreams.takeFirst();
}

void Port::updatePortConfig(OstProto::Port *port)
{
    d.MergeFrom(*port);
}

void Port::updateStreamOrdinalsFromIndex()
{
    for (int i=0; i < mStreams.size(); i++)
        mStreams[i]->setOrdinal(i);
}

void Port::reorderStreamsByOrdinals()
{
    qSort(mStreams.begin(), mStreams.end(), StreamBase::StreamLessThan);
}

bool Port::newStreamAt(int index, OstProto::Stream const *stream)
{
    Stream    *s = new Stream;

    if (index > mStreams.size())
        return false;

    if (stream)
        s->protoDataCopyFrom(*stream);

    s->setId(newStreamId());
    mStreams.insert(index, s);
    updateStreamOrdinalsFromIndex();

    return true;
}

bool Port::deleteStreamAt(int index)
{
    if (index >= mStreams.size())
        return false;

    delete mStreams.takeAt(index);
    updateStreamOrdinalsFromIndex();

    return true;
}

bool Port::insertStream(uint streamId)
{
    Stream    *s = new Stream;

    s->setId(streamId);

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
        if (streamId == mStreams[i]->id())
            goto _found;
    }

    qDebug("%s: Invalid stream id %d", __FUNCTION__, streamId);
    return false;

_found:
    streamIndex = i;

    mStreams[streamIndex]->protoDataCopyFrom(*stream);
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
            if (mLastSyncStreamList[i] == mStreams[j]->id())
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
            OstProto::StreamId    *s;

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
        if (mLastSyncStreamList.contains(mStreams[i]->id()))
        {
            // existing stream!
            continue;
        }
        else
        {
            // new stream!
            OstProto::StreamId    *s;

            s = streamIdList.add_stream_id();
            s->set_id(mStreams[i]->id());
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
        OstProto::Stream    *s;

        s = streamConfigList.add_stream();
        mStreams[i]->protoDataCopyInto(*s);
    }
    qDebug("Done %s", __FUNCTION__);
}

void Port::when_syncComplete()
{
    //reorderStreamsByOrdinals();

    mLastSyncStreamList.clear();
    for (int i=0; i<mStreams.size(); i++)
        mLastSyncStreamList.append(mStreams[i]->id());
}

void Port::updateStats(OstProto::PortStats *portStats)
{
    OstProto::PortState        oldState;

    oldState = stats.state(); 
    stats.MergeFrom(*portStats);

    if (oldState.link_state() != stats.state().link_state())
    {
        qDebug("portstate changed");
        emit portDataChanged(mPortGroupId, mPortId);
    }
}

bool Port::openStreams(QString fileName, bool append, QString &error)
{
    OstProto::StreamConfigList streams;
    AbstractFileFormat *fmt = AbstractFileFormat::fileFormatFromFile(fileName);

    if (fmt == NULL)
        goto _fail;

    if (!fmt->openStreams(fileName, streams, error))
        goto _fail;

    if (!append)
    {
        while (numStreams())
            deleteStreamAt(0);
    }

    for (int i = 0; i < streams.stream_size(); i++)
    {
        newStreamAt(mStreams.size(), &streams.stream(i));
    }

    emit streamListChanged(mPortGroupId, mPortId);

    return true;

_fail:
    return false;
}

bool Port::saveStreams(QString fileName, QString fileType, QString &error)
{
    AbstractFileFormat *fmt = AbstractFileFormat::fileFormatFromType(fileType);
    OstProto::StreamConfigList streams;

    if (fmt == NULL)
        goto _fail;

    streams.mutable_port_id()->set_id(0);
    for (int i = 0; i < mStreams.size(); i++)
    {
        OstProto::Stream *s = streams.add_stream();
        mStreams[i]->protoDataCopyInto(*s);
    }

    return fmt->saveStreams(streams, fileName, error);

_fail:
    error = QString("Unsupported File Type - %1").arg(fileType);
    return false;
}
