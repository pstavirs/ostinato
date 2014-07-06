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
#include <QMainWindow>
#include <QProgressDialog>
#include <QVariant>
#include <google/protobuf/descriptor.h>
#include <vector>

extern QMainWindow *mainWindow;

uint Port::mAllocStreamId = 0;

static const int kEthOverhead = 20;

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

void Port::recalculateAverageRates()
{
    double pps = 0;
    double bps = 0;
    int n = 0;

    foreach (Stream* s, mStreams)
    {
        if (!s->isEnabled())
            continue;

        double r = s->averagePacketRate();
        pps += r;
        bps += r * (s->frameLenAvg() + kEthOverhead) * 8;
        n++;

        if ((transmitMode() == OstProto::kSequentialTransmit) 
                && (s->nextWhat() == Stream::e_nw_stop))
            break;
    }

    if (n)
    {
        switch (transmitMode())
        {
        case OstProto::kSequentialTransmit:
            avgPacketsPerSec_ = pps/n;
            avgBitsPerSec_ = bps/n;
            break;
        case OstProto::kInterleavedTransmit:
            avgPacketsPerSec_ = pps;
            avgBitsPerSec_ = bps;
            break;
        default:
            Q_ASSERT(false); // Unreachable!!
        }
        numActiveStreams_ = n;
    }
    else
        avgPacketsPerSec_ = avgBitsPerSec_ = numActiveStreams_ = 0;

    qDebug("%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);

    emit portRateChanged(mPortGroupId, mPortId);

}

void Port::setAveragePacketRate(double packetsPerSec)
{
    double rate = 0;
    double pps = 0;
    double bps = 0;
    int n = 0;

    qDebug("@%s: packetsPerSec = %g", __FUNCTION__, packetsPerSec);
    qDebug("@%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);
    foreach (Stream* s, mStreams)
    {
        if (!s->isEnabled())
            continue;

        switch (transmitMode())
        {
        case OstProto::kSequentialTransmit:
            rate = s->averagePacketRate() * (packetsPerSec/avgPacketsPerSec_);
            break;
        case OstProto::kInterleavedTransmit:
            rate = s->averagePacketRate() + 
                ((s->averagePacketRate()/avgPacketsPerSec_) * 
                 (packetsPerSec - avgPacketsPerSec_));
            break;
        default:
            Q_ASSERT(false); // Unreachable!!
        }

        qDebug("cur stream pps = %g", s->averagePacketRate());

        s->setAveragePacketRate(rate);

        qDebug("new stream pps = %g", s->averagePacketRate());

        double r = s->averagePacketRate();
        pps += r;
        bps += r * (s->frameLenAvg() + kEthOverhead) * 8;
        n++;

        if ((transmitMode() == OstProto::kSequentialTransmit) 
                && (s->nextWhat() == Stream::e_nw_stop))
            break;
    }

    if (n)
    {
        switch (transmitMode())
        {
        case OstProto::kSequentialTransmit:
            avgPacketsPerSec_ = pps/n;
            avgBitsPerSec_ = bps/n;
            break;
        case OstProto::kInterleavedTransmit:
            avgPacketsPerSec_ = pps;
            avgBitsPerSec_ = bps;
            break;
        default:
            Q_ASSERT(false); // Unreachable!!
        }
        numActiveStreams_ = n;
    }
    else
        avgPacketsPerSec_ = avgBitsPerSec_ = numActiveStreams_ = 0;

    qDebug("%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);

    emit portRateChanged(mPortGroupId, mPortId);
}

void Port::setAverageBitRate(double bitsPerSec)
{
    double rate;
    double pps = 0;
    double bps = 0;
    int n = 0;

    qDebug("@%s: bitsPerSec = %g", __FUNCTION__, bitsPerSec);
    qDebug("@%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);
    foreach (Stream* s, mStreams)
    {
        if (!s->isEnabled())
            continue;

        switch (transmitMode())
        {
        case OstProto::kSequentialTransmit:
            rate = s->averagePacketRate() * (bitsPerSec/avgBitsPerSec_);
            qDebug("rate = %g", rate);
            break;
        case OstProto::kInterleavedTransmit:
            rate = s->averagePacketRate() 
                + ((s->averagePacketRate()/avgPacketsPerSec_) 
                        * ((bitsPerSec - avgBitsPerSec_)
                            / ((s->frameLenAvg()+kEthOverhead)*8)));
            break;
        default:
            Q_ASSERT(false); // Unreachable!!
        }

        qDebug("cur stream pps = %g", s->averagePacketRate());

        s->setAveragePacketRate(rate);

        qDebug("new stream pps = %g", s->averagePacketRate());

        double r = s->averagePacketRate();
        pps += r;
        bps += r * (s->frameLenAvg() + kEthOverhead) * 8;
        n++;

        if ((transmitMode() == OstProto::kSequentialTransmit) 
                && (s->nextWhat() == Stream::e_nw_stop))
            break;
    }

    if (n)
    {
        switch (transmitMode())
        {
        case OstProto::kSequentialTransmit:
            avgPacketsPerSec_ = pps/n;
            avgBitsPerSec_ = bps/n;
            break;
        case OstProto::kInterleavedTransmit:
            avgPacketsPerSec_ = pps;
            avgBitsPerSec_ = bps;
            break;
        default:
            Q_ASSERT(false); // Unreachable!!
        }
        numActiveStreams_ = n;
    }
    else
        avgPacketsPerSec_ = avgBitsPerSec_ = numActiveStreams_ = 0;

    qDebug("%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);
    emit portRateChanged(mPortGroupId, mPortId);
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
    recalculateAverageRates();

    return true;
}

bool Port::deleteStreamAt(int index)
{
    if (index >= mStreams.size())
        return false;

    delete mStreams.takeAt(index);
    updateStreamOrdinalsFromIndex();
    recalculateAverageRates();

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
    bool ret = false; 
    QDialog *optDialog;
    QProgressDialog progress("Opening Streams", "Cancel", 0, 0, mainWindow);
    OstProto::StreamConfigList streams;
    AbstractFileFormat *fmt = AbstractFileFormat::fileFormatFromFile(fileName);

    if (fmt == NULL)
        goto _fail;

    if ((optDialog = fmt->openOptionsDialog()))
    {
        int ret;
        optDialog->setParent(mainWindow, Qt::Dialog);
        ret = optDialog->exec();
        optDialog->setParent(0, Qt::Dialog);
        if (ret == QDialog::Rejected)
            goto _user_opt_cancel;
    }

    progress.setAutoReset(false);
    progress.setAutoClose(false);
    progress.setMinimumDuration(0);
    progress.show();

    mainWindow->setDisabled(true);
    progress.setEnabled(true); // to override the mainWindow disable

    connect(fmt, SIGNAL(status(QString)),&progress,SLOT(setLabelText(QString)));
    connect(fmt, SIGNAL(target(int)), &progress, SLOT(setMaximum(int)));
    connect(fmt, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    connect(&progress, SIGNAL(canceled()), fmt, SLOT(cancel()));

    fmt->openStreamsOffline(fileName, streams, error);
    qDebug("after open offline");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for offline operation");

    if (!fmt->result())
        goto _fail;
    
    // process any remaining events posted from the thread
    for (int i = 0; i < 10; i++)
        qApp->processEvents();

    if (!append)
    {
        int n = numStreams();

        progress.setLabelText("Deleting existing streams...");
        progress.setRange(0, n);
        for (int i = 0; i < n; i++)
        {
            if (progress.wasCanceled())
                goto _user_cancel;
            deleteStreamAt(0);
            progress.setValue(i);
            if (i % 32 == 0)
                qApp->processEvents();
        }
    }

    progress.setLabelText("Constructing new streams...");
    progress.setRange(0, streams.stream_size());
    for (int i = 0; i < streams.stream_size(); i++)
    {
        if (progress.wasCanceled())
            goto _user_cancel;
        newStreamAt(mStreams.size(), &streams.stream(i));
        progress.setValue(i);
        if (i % 32 == 0)
            qApp->processEvents();
    }

_user_cancel:
    emit streamListChanged(mPortGroupId, mPortId);
_user_opt_cancel:
    ret = true;

_fail:
    progress.close();
    mainWindow->setEnabled(true);
    recalculateAverageRates();
    return ret;
}

bool Port::saveStreams(QString fileName, QString fileType, QString &error)
{
    bool ret = false;
    QProgressDialog progress("Saving Streams", "Cancel", 0, 0, mainWindow);
    AbstractFileFormat *fmt = AbstractFileFormat::fileFormatFromType(fileType);
    OstProto::StreamConfigList streams;

    if (fmt == NULL)
        goto _fail;

    progress.setAutoReset(false);
    progress.setAutoClose(false);
    progress.setMinimumDuration(0);
    progress.show();

    mainWindow->setDisabled(true);
    progress.setEnabled(true); // to override the mainWindow disable

    connect(fmt, SIGNAL(status(QString)),&progress,SLOT(setLabelText(QString)));
    connect(fmt, SIGNAL(target(int)), &progress, SLOT(setMaximum(int)));
    connect(fmt, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    connect(&progress, SIGNAL(canceled()), fmt, SLOT(cancel()));

    progress.setLabelText("Preparing Streams...");
    progress.setRange(0, mStreams.size());
    streams.mutable_port_id()->set_id(0);
    for (int i = 0; i < mStreams.size(); i++)
    {
        OstProto::Stream *s = streams.add_stream();
        mStreams[i]->protoDataCopyInto(*s);

        if (progress.wasCanceled())
            goto _user_cancel;
        progress.setValue(i);
        if (i % 32 == 0)
            qApp->processEvents();
    }

    fmt->saveStreamsOffline(streams, fileName, error);
    qDebug("after save offline");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for offline operation");

    ret = fmt->result();
    goto _exit;

_user_cancel:
   goto _exit; 

_fail:
    error = QString("Unsupported File Type - %1").arg(fileType);
    goto _exit;

_exit:
    progress.close();
    mainWindow->setEnabled(true);
    return ret;
}
