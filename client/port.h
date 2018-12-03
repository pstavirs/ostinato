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

#ifndef _PORT_H
#define _PORT_H

#include <QDir>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QTemporaryFile>

#include "stream.h"

//class StreamModel;
namespace OstEmul {
    class Device;
    class DeviceNeighborList;
}

class Port : public QObject {

    Q_OBJECT

    static uint            mAllocStreamId;
    static uint            allocDeviceGroupId_;

    OstProto::Port        d;
    OstProto::PortStats   stats;
    QTemporaryFile *capFile_;

    // FIXME(HI): consider removing mPortId as it is duplicated inside 'd'
    quint32        mPortId;
    quint32        mPortGroupId;
    QString        mUserAlias;            // user defined
    bool           dirty_;

    double avgPacketsPerSec_; 
    double avgBitsPerSec_;
    int numActiveStreams_;

    QList<quint32>    mLastSyncStreamList;
    QList<Stream*>    mStreams;        // sorted by stream's ordinal value

    QList<quint32> lastSyncDeviceGroupList_;
    QSet<quint32>  modifiedDeviceGroupList_;
    QList<OstProto::DeviceGroup*> deviceGroups_;
    QList<OstEmul::Device*> devices_;
    QHash<quint32, OstEmul::DeviceNeighborList*> deviceNeighbors_;
    QHash<quint32, quint32> arpResolvedCount_;
    QHash<quint32, quint32> ndpResolvedCount_;

    uint newStreamId();
    void updateStreamOrdinalsFromIndex();
    void reorderStreamsByOrdinals();

    void setDirty(bool dirty);

public:
    enum AdminStatus    { AdminDisable, AdminEnable };

    // FIXME(HIGH): default args is a hack for QList operations on Port
    Port(quint32 id = 0xFFFFFFFF, quint32 pgId = 0xFFFFFFFF);
    ~Port();

    quint32 portGroupId() const { return mPortGroupId; }
    const QString userAlias() const
        { return mUserAlias.isEmpty() ? name() :  mUserAlias; }

    quint32 id() const 
        { return d.port_id().id(); }
    const QString name() const 
        { return QString().fromStdString(d.name()); }
    const QString description() const 
        { return QString().fromStdString(d.description()); }
    const QString notes() const 
        { return QString().fromStdString(d.notes()); }
    const QString userName() const 
        { return QString().fromStdString(d.user_name()); }
    AdminStatus adminStatus() const
        { return (d.is_enabled()?AdminEnable:AdminDisable); }
    bool hasExclusiveControl() const
        { return d.is_exclusive_control(); }
    OstProto::TransmitMode transmitMode() const
        { return d.transmit_mode(); }
    bool trackStreamStats() const
        { return d.is_tracking_stream_stats(); }
    double averagePacketRate() const
        { return avgPacketsPerSec_; }
    double averageBitRate() const
        { return avgBitsPerSec_; }

    //void setAdminEnable(AdminStatus status) { mAdminStatus = status; }
    void setAlias(QString alias) { mUserAlias = alias; }
    //void setExclusive(bool flag);

    int numStreams() { return mStreams.size(); }
    const Stream* streamByIndex(int index) const
    {
        Q_ASSERT(index < mStreams.size());
        return mStreams[index];
    }
    Stream* mutableStreamByIndex(int index, bool assumeChange = true)
    {
        Q_ASSERT(index < mStreams.size());
        if (assumeChange)
            setDirty(true);
        return mStreams[index];
    }
    void setLocalConfigChanged(bool changed)
    {
        setDirty(changed);
    }
    OstProto::LinkState linkState()
        { return stats.state().link_state(); }
    bool isTransmitting()
        { return stats.state().is_transmit_on(); }
    bool isCapturing()
        { return stats.state().is_capture_on(); }

    OstProto::PortStats    getStats() { return stats; }
    QTemporaryFile* getCaptureFile() 
    {
        delete capFile_;
        capFile_ = new QTemporaryFile(QString(QDir::tempPath())
                                        .append("/")
                                        .append(userAlias())
                                        .append(".XXXXXX"));
        return capFile_; 
    }

    void protoDataCopyInto(OstProto::Port *data);

    //! Used when config received from server
    // FIXME(MED): naming inconsistency - PortConfig/Stream; also retVal
    void updatePortConfig(OstProto::Port *port);
    
    //! Used by StreamModel
    //@{
    bool newStreamAt(int index, OstProto::Stream const *stream = NULL);
    bool deleteStreamAt(int index);
    //@}

    //! Used by MyService::Stub to update from config received from server
    //@{
    bool insertStream(uint streamId);
    bool updateStream(uint streamId, OstProto::Stream *stream);
    //@}

    bool isDirty() { return dirty_; }
    void getDeletedStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
    void getNewStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
    void getModifiedStreamsSinceLastSync(
        OstProto::StreamConfigList &streamConfigList);

    void getDeletedDeviceGroupsSinceLastSync(
            OstProto::DeviceGroupIdList &streamIdList);
    void getNewDeviceGroupsSinceLastSync(
            OstProto::DeviceGroupIdList &streamIdList);
    void getModifiedDeviceGroupsSinceLastSync(
            OstProto::DeviceGroupConfigList &streamConfigList);

    bool modifiablePortConfig(OstProto::Port &config) const;

    void when_syncComplete();

    void setAveragePacketRate(double packetsPerSec);
    void setAverageBitRate(double bitsPerSec);
    // FIXME(MED): Bad Hack! port should not need an external trigger to
    // recalculate - refactor client side domain objects and model objects
    void recalculateAverageRates();
    void updateStats(OstProto::PortStats *portStats);

    void duplicateStreams(const QList<int> &list, int count);

    bool openStreams(QString fileName, bool append, QString &error);
    bool saveStreams(QString fileName, QString fileType, QString &error);

    // ------------ Device Group ----------- //

    uint newDeviceGroupId();
    int numDeviceGroups() const;
    const OstProto::DeviceGroup* deviceGroupByIndex(int index) const;
    OstProto::DeviceGroup* mutableDeviceGroupByIndex(int index);
    const OstProto::DeviceGroup* deviceGroupById(uint deviceGroupId) const;

    //! Used by StreamModel
    //@{
    bool newDeviceGroupAt(int index,
                          const OstProto::DeviceGroup *deviceGroup = NULL);
    bool deleteDeviceGroupAt(int index);
    //@}

    //! Used by MyService::Stub to update from config received from server
    //@{
    bool insertDeviceGroup(uint deviceGroupId);
    bool updateDeviceGroup(uint deviceGroupId,
                           OstProto::DeviceGroup *deviceGroup);
    //@}

    // ------------ Device ----------- //

    int numDevices();
    const OstEmul::Device* deviceByIndex(int index);

    //! Used by MyService::Stub to update from config received from server
    void clearDeviceList();
    void insertDevice(const OstEmul::Device &device);

    const OstEmul::DeviceNeighborList* deviceNeighbors(int deviceIndex);
    int numArp(int deviceIndex);
    int numArpResolved(int deviceIndex);
    int numNdp(int deviceIndex);
    int numNdpResolved(int deviceIndex);

    //! Used by MyService::Stub to update from config received from server
    void clearDeviceNeighbors();
    void insertDeviceNeighbors(const OstEmul::DeviceNeighborList &neighList);

    void deviceInfoRefreshed();

signals:
    //! Used when local config changed and when config received from server
    void portRateChanged(int portGroupId, int portId);

    //! Used by MyService::Stub to update from config received from server
    //@{
    void portDataChanged(int portGroupId, int portId);
    void deviceInfoChanged();
    //@}

    //! Used when local config changed
    //@{
    void streamListChanged(int portGroupId, int portId);
    void localConfigChanged(int portGroupId, int portId, bool changed);
    //@}
};

#endif
