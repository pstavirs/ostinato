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

#include <QList>
#include <QString>
#include <QTemporaryFile>

#include "stream.h"

//class StreamModel;

class Port : public QObject {

    Q_OBJECT

    static uint            mAllocStreamId;

    OstProto::Port        d;
    OstProto::PortStats   stats;
    QTemporaryFile *capFile_;

    // FIXME(HI): consider removing mPortId as it is duplicated inside 'd'
    quint32        mPortId;
    quint32        mPortGroupId;
    QString        mUserAlias;            // user defined

    double avgPacketsPerSec_; 
    double avgBitsPerSec_; 

    QList<quint32>    mLastSyncStreamList;
    QList<Stream*>    mStreams;        // sorted by stream's ordinal value

    uint newStreamId();
    void updateStreamOrdinalsFromIndex();
    void reorderStreamsByOrdinals();


public:
    enum AdminStatus    { AdminDisable, AdminEnable };

    // FIXME(HIGH): default args is a hack for QList operations on Port
    Port(quint32 id = 0xFFFFFFFF, quint32 pgId = 0xFFFFFFFF);
    ~Port();

    quint32 portGroupId() const { return mPortGroupId; }
    const QString& userAlias() const { return mUserAlias; }

    quint32 id() const 
        { return d.port_id().id(); }
    const QString name() const 
        { return QString().fromStdString(d.name()); }
    const QString description() const 
        { return QString().fromStdString(d.description()); }
    const QString notes() const 
        { return QString().fromStdString(d.notes()); }
    AdminStatus adminStatus() 
        { return (d.is_enabled()?AdminEnable:AdminDisable); }
    bool hasExclusiveControl() 
        { return d.is_exclusive_control(); }
    double averagePacketRate()
        { return avgPacketsPerSec_; }
    double averageBitRate()
        { return avgBitsPerSec_; }

    //void setAdminEnable(AdminStatus status) { mAdminStatus = status; }
    void setAlias(QString &alias) { mUserAlias = alias; }
    //void setExclusive(bool flag);

    int numStreams() { return mStreams.size(); }
    Stream* streamByIndex(int index)
    {
        Q_ASSERT(index < mStreams.size());
        return mStreams[index];
    }
    OstProto::LinkState linkState()
        { return stats.state().link_state(); }

    OstProto::PortStats    getStats() { return stats; }
    QTemporaryFile* getCaptureFile() 
    {
        delete capFile_;
        capFile_ = new QTemporaryFile();
        return capFile_; 
    }

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

    void getDeletedStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
    void getNewStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
    void getModifiedStreamsSinceLastSync(
        OstProto::StreamConfigList &streamConfigList);

    void when_syncComplete();

    void setAveragePacketRate(double packetsPerSec);
    void setAverageBitRate(double bitsPerSec);
    // FIXME(MED): Bad Hack! port should not need an external trigger to
    // recalculate - refactor client side domain objects and model objects
    void recalculateAverageRates();
    void updateStats(OstProto::PortStats *portStats);

    bool openStreams(QString fileName, bool append, QString &error);
    bool saveStreams(QString fileName, QString fileType, QString &error);

signals:
    void portRateChanged(int portGroupId, int portId);
    void portDataChanged(int portGroupId, int portId);
    void streamListChanged(int portGroupId, int portId);

};

#endif
