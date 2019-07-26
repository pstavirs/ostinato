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

#include "emulation.h"
#include "streamfileformat.h"

#include <QApplication>
#include <QMainWindow>
#include <QProgressDialog>
#include <QVariant>
#include <google/protobuf/descriptor.h>
#include <vector>

extern QMainWindow *mainWindow;

uint Port::mAllocStreamId = 0;
uint Port::allocDeviceGroupId_ = 1;

static const int kEthOverhead = 20;

uint Port::newStreamId()
{
    // We use a monotonically increasing class variable to generate
    // unique id. To ensure that we take into account the already
    // existing ids at drone, we update this variable to be greater
    // than the last id that we fetched
    // FIXME: In some cases e.g. wrap around or more likely multiple
    // clients, we will still run into trouble - to fix this we should
    // check here that the same id does not already exist; but currently
    // we can only do a linear search - fix this when the lookup/search
    // is optimized
    return mAllocStreamId++;
}

Port::Port(quint32 id, quint32 portGroupId)
{
    mPortId = id;
    d.mutable_port_id()->set_id(id);
    stats.mutable_port_id()->set_id(id);
    mPortGroupId = portGroupId;
    capFile_ = NULL;
    dirty_ = false;
}

Port::~Port()
{
    qDebug("%s", __FUNCTION__);
    while (!mStreams.isEmpty())
        delete mStreams.takeFirst();
}

void Port::protoDataCopyInto(OstProto::Port *data)
{
    data->CopyFrom(d);
}

void Port::updatePortConfig(OstProto::Port *port)
{
    bool recalc = false;

    if (port->has_transmit_mode() 
            && port->transmit_mode() != d.transmit_mode())
        recalc = true;

    d.MergeFrom(*port);

    // Setup a user-friendly alias for Win32 ports
    if (name().startsWith("\\Device\\NPF_"))
        setAlias(QString("if%1").arg(id()));

    if (recalc)
        recalculateAverageRates();
}

void Port::updateStreamOrdinalsFromIndex()
{
    for (int i=0; i < mStreams.size(); i++)
        mStreams[i]->setOrdinal(i);
}

void Port::reorderStreamsByOrdinals()
{
    std::sort(mStreams.begin(), mStreams.end(), StreamBase::StreamLessThan);
}

void Port::setDirty(bool dirty)
{
    if (dirty == dirty_)
        return;

    dirty_ = dirty;
    emit localConfigChanged(mPortGroupId, mPortId, dirty_);
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
        setDirty(true);
    }
    else
        avgPacketsPerSec_ = avgBitsPerSec_ = numActiveStreams_ = 0;

    qDebug("%s: avgPps = %g avgBps = %g numActive = %d", __FUNCTION__,
            avgPacketsPerSec_, avgBitsPerSec_, numActiveStreams_);

    emit portRateChanged(mPortGroupId, mPortId);
}

void Port::setAverageBitRate(double bitsPerSec)
{
    double rate = 0;
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
        setDirty(true);
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
    setDirty(true);

    return true;
}

bool Port::deleteStreamAt(int index)
{
    if (index >= mStreams.size())
        return false;

    delete mStreams.takeAt(index);
    updateStreamOrdinalsFromIndex();
    recalculateAverageRates();
    setDirty(true);

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

void Port::getDeletedDeviceGroupsSinceLastSync(
    OstProto::DeviceGroupIdList &deviceGroupIdList)
{
    deviceGroupIdList.clear_device_group_id();
    foreach(int id, lastSyncDeviceGroupList_) {
        if (!deviceGroupById(id))
            deviceGroupIdList.add_device_group_id()->set_id(id);
    }
}

void Port::getNewDeviceGroupsSinceLastSync(
    OstProto::DeviceGroupIdList &deviceGroupIdList)
{
    deviceGroupIdList.clear_device_group_id();
    foreach(OstProto::DeviceGroup *dg, deviceGroups_) {
        quint32 dgid = dg->device_group_id().id();
        if (!lastSyncDeviceGroupList_.contains(dgid))
            deviceGroupIdList.add_device_group_id()->set_id(dgid);
    }
}

void Port::getModifiedDeviceGroupsSinceLastSync(
    OstProto::DeviceGroupConfigList &deviceGroupConfigList)
{
    deviceGroupConfigList.clear_device_group();
    foreach(quint32 id, modifiedDeviceGroupList_)
        deviceGroupConfigList.add_device_group()
                                ->CopyFrom(*deviceGroupById(id));
}

/*!
 * Finds the user modifiable fields in 'config' that are different from
 * the current configuration on the port and modifes 'config' such that
 * only those fields are set and returns true. If 'config' is same as
 * current port config, 'config' is unmodified and false is returned
 */
bool Port::modifiablePortConfig(OstProto::Port &config) const
{
    bool change = false;
    OstProto::Port modCfg;

    if (config.is_exclusive_control() != d.is_exclusive_control()) {
        modCfg.set_is_exclusive_control(config.is_exclusive_control());
        change = true;
    }
    if (config.transmit_mode() != d.transmit_mode()) {
        modCfg.set_transmit_mode(config.transmit_mode());
        change = true;
    }
    if (config.user_name() != d.user_name()) {
        modCfg.set_user_name(config.user_name());
        change = true;
    }

    if (change) {
        modCfg.mutable_port_id()->set_id(id());
        config.CopyFrom(modCfg);

        return true;
    }

    return false;
}

void Port::when_syncComplete()
{
    //reorderStreamsByOrdinals();

    mLastSyncStreamList.clear();
    for (int i=0; i<mStreams.size(); i++)
        mLastSyncStreamList.append(mStreams[i]->id());

    lastSyncDeviceGroupList_.clear();
    for (int i = 0; i < deviceGroups_.size(); i++) {
        lastSyncDeviceGroupList_.append(
                deviceGroups_.at(i)->device_group_id().id());
    }
    modifiedDeviceGroupList_.clear();

    setDirty(false);
}

void Port::updateStats(OstProto::PortStats *portStats)
{
    OstProto::PortState        oldState;

    oldState = stats.state(); 
    stats.MergeFrom(*portStats);

    if ((oldState.link_state() != stats.state().link_state())
        || (oldState.is_transmit_on() != stats.state().is_transmit_on())
        || (oldState.is_capture_on() != stats.state().is_capture_on()))
    {
        qDebug("portstate changed");
        emit portDataChanged(mPortGroupId, mPortId);
    }
}

void Port::duplicateStreams(const QList<int> &list, int count)
{
    QList<OstProto::Stream> sources;
    foreach(int index, list) {
        OstProto::Stream stream;
        Q_ASSERT(index < mStreams.size());
        mStreams.at(index)->protoDataCopyInto(stream);
        sources.append(stream);
    }

    int insertAt = mStreams.size();
    for (int i=0; i < count; i++) {
        for (int j=0; j < sources.size(); j++) {
            newStreamAt(insertAt, &sources.at(j));
            // Rename stream by appending the copy count
            mStreams.at(insertAt)->setName(QString("%1 (%2)")
                    .arg(mStreams.at(insertAt)->name())
                    .arg(i+1));
            insertAt++;
        }
    }
    setDirty(true);

    emit streamListChanged(mPortGroupId, mPortId);
}

bool Port::openStreams(QString fileName, bool append, QString &error)
{
    bool ret = false; 
    QDialog *optDialog;
    QProgressDialog progress("Opening Streams", "Cancel", 0, 0, mainWindow);
    OstProto::StreamConfigList streams;
    StreamFileFormat *fmt = StreamFileFormat::fileFormatFromFile(fileName);

    if (fmt == NULL) {
        error = tr("Unknown streams file format");
        goto _fail;
    }

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

    fmt->openAsync(fileName, streams, error);
    qDebug("after open async");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for async operation");

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
    setDirty(true);

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
    StreamFileFormat *fmt = StreamFileFormat::fileFormatFromType(fileType);
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

    fmt->saveAsync(streams, fileName, error);
    qDebug("after save async");

    while (!fmt->isFinished())
        qApp->processEvents();
    qDebug("wait over for async operation");

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

// ------------ Device Group ----------- //

uint Port::newDeviceGroupId()
{
    // We use a monotonically increasing class variable to generate
    // unique id. To ensure that we take into account the already
    // existing ids at drone, we update this variable to be greater
    // than the last id that we fetched
    // FIXME: In some cases e.g. wrap around or more likely multiple
    // clients, we will still run into trouble - to fix this we should
    // check here that the same id does not already exist; but currently
    // we can only do a linear search - fix this when the lookup/search
    // is optimized
    return allocDeviceGroupId_++;
}

int Port::numDeviceGroups() const
{
    return deviceGroups_.size();
}

const OstProto::DeviceGroup* Port::deviceGroupByIndex(int index) const
{
    if ((index < 0) || (index >= numDeviceGroups())) {
        qWarning("%s: index %d out of range (0 - %d)", __FUNCTION__,
                index, numDeviceGroups() - 1);
        return NULL;
    }

    return deviceGroups_.at(index);
}

OstProto::DeviceGroup* Port::mutableDeviceGroupByIndex(int index)
{
    if ((index < 0) || (index >= numDeviceGroups())) {
        qWarning("%s: index %d out of range (0 - %d)", __FUNCTION__,
                index, numDeviceGroups() - 1);
        return NULL;
    }

    OstProto::DeviceGroup *devGrp = deviceGroups_.at(index);

    // Caller can modify DeviceGroup - assume she will
    modifiedDeviceGroupList_.insert(devGrp->device_group_id().id());
    setDirty(true);

    return devGrp;
}

const OstProto::DeviceGroup* Port::deviceGroupById(uint deviceGroupId) const
{
    for (int i = 0; i < deviceGroups_.size(); i++) {
        OstProto::DeviceGroup *devGrp = deviceGroups_.at(i);
        if (devGrp->device_group_id().id() == deviceGroupId)
            return devGrp;
    }

    return NULL;
}

bool Port::newDeviceGroupAt(int index, const OstProto::DeviceGroup *deviceGroup)
{
    if (index < 0 || index > numDeviceGroups())
        return false;

    OstProto::DeviceGroup *devGrp = newDeviceGroup(id());

    if (!devGrp) {
        qWarning("failed allocating a new device group");
        return false;
    }

    if (deviceGroup)
        devGrp->CopyFrom(*deviceGroup);

    devGrp->mutable_device_group_id()->set_id(newDeviceGroupId());
    deviceGroups_.insert(index, devGrp);
    modifiedDeviceGroupList_.insert(devGrp->device_group_id().id());
    setDirty(true);

    return true;
}

bool Port::deleteDeviceGroupAt(int index)
{
    if (index < 0 || index >= deviceGroups_.size())
        return false;

    OstProto::DeviceGroup *devGrp = deviceGroups_.takeAt(index);
    modifiedDeviceGroupList_.remove(devGrp->device_group_id().id());
    delete devGrp;
    setDirty(true);

    return true;
}

bool Port::insertDeviceGroup(uint deviceGroupId)
{
    OstProto::DeviceGroup *devGrp;

    if (deviceGroupById(deviceGroupId)) {
        qDebug("%s: deviceGroup id %u already exists", __FUNCTION__,
                deviceGroupId);
        return false;
    }

    devGrp = newDeviceGroup(id());
    devGrp->mutable_device_group_id()->set_id(deviceGroupId);
    deviceGroups_.append(devGrp);

    // Update allocDeviceGroupId_ to take into account the deviceGroup id
    // received from server - this is required to make sure newly allocated
    // deviceGroup ids are unique
    if (allocDeviceGroupId_ <= deviceGroupId)
        allocDeviceGroupId_ = deviceGroupId + 1;
    return true;
}

bool Port::updateDeviceGroup(
        uint deviceGroupId,
        OstProto::DeviceGroup *deviceGroup)
{
    using OstProto::DeviceGroup;

    // XXX: We should not call mutableDeviceGroupById() because that will
    // implicitly set the port as dirty, so we use a const_cast hack instead
    DeviceGroup *devGrp = const_cast<DeviceGroup*>(
                                deviceGroupById(deviceGroupId));

    if (!devGrp) {
        qDebug("%s: deviceGroup id %u does not exist", __FUNCTION__,
                deviceGroupId);
        return false;
    }

    devGrp->CopyFrom(*deviceGroup);
    return true;
}

// ------------ Devices  ----------- //

int Port::numDevices()
{
    return devices_.size();
}

const OstEmul::Device* Port::deviceByIndex(int index)
{
    if ((index < 0) || (index >= numDevices())) {
        qWarning("%s: index %d out of range (0 - %d)", __FUNCTION__,
                index, numDevices() - 1);
        return NULL;
    }

    return devices_.at(index);
}

void Port::clearDeviceList()
{
    while (devices_.size())
        delete devices_.takeFirst();
}

void Port::insertDevice(const OstEmul::Device &device)
{
    OstEmul::Device *dev = new OstEmul::Device(device);
    devices_.append(dev);
}

// ------------- Device Neighbors (ARP/NDP) ------------- //

const OstEmul::DeviceNeighborList* Port::deviceNeighbors(int deviceIndex)
{
    if ((deviceIndex < 0) || (deviceIndex >= numDevices())) {
        qWarning("%s: index %d out of range (0 - %d)", __FUNCTION__,
                deviceIndex, numDevices() - 1);
        return NULL;
    }

    return deviceNeighbors_.value(deviceIndex);
}

int Port::numArp(int deviceIndex)
{
    if (deviceNeighbors_.contains(deviceIndex))
        return deviceNeighbors_.value(deviceIndex)->arp_size();

    return 0;
}

int Port::numArpResolved(int deviceIndex)
{
    if (arpResolvedCount_.contains(deviceIndex))
        return arpResolvedCount_.value(deviceIndex);

    return 0;
}

int Port::numNdp(int deviceIndex)
{
    if (deviceNeighbors_.contains(deviceIndex))
        return deviceNeighbors_.value(deviceIndex)->ndp_size();

    return 0;
}

int Port::numNdpResolved(int deviceIndex)
{
    if (ndpResolvedCount_.contains(deviceIndex))
        return ndpResolvedCount_.value(deviceIndex);

    return 0;
}

void Port::clearDeviceNeighbors()
{
    arpResolvedCount_.clear();
    ndpResolvedCount_.clear();
    qDeleteAll(deviceNeighbors_);
    deviceNeighbors_.clear();
}

void Port::insertDeviceNeighbors(const OstEmul::DeviceNeighborList &neighList)
{
    int count;
    OstEmul::DeviceNeighborList *neighbors =
        new OstEmul::DeviceNeighborList(neighList);
    deviceNeighbors_.insert(neighList.device_index(), neighbors);

    count = 0;
    for (int i = 0; i < neighbors->arp_size(); i++)
        if (neighbors->arp(i).mac())
            count++;
    arpResolvedCount_.insert(neighbors->device_index(), count);

    count = 0;
    for (int i = 0; i < neighbors->ndp_size(); i++)
        if (neighbors->ndp(i).mac())
            count++;
    ndpResolvedCount_.insert(neighbors->device_index(), count);
}

void Port::deviceInfoRefreshed()
{
    emit deviceInfoChanged();
}

