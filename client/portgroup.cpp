#include <QtGlobal>
#include <QProcess>
#include <QTemporaryFile>

#include "portgroup.h"


quint32    PortGroup::mPortGroupAllocId = 0;

PortGroup::PortGroup(QHostAddress ip, quint16 port)
{
    // Allocate an id for self
    mPortGroupId = PortGroup::mPortGroupAllocId++;

    rpcChannel = new PbRpcChannel(ip, port);

    /*! 
      \todo (HIGH) RPC Controller should be allocated and deleted for each RPC invocation
      as implemented currently, if a RPC is invoked before the previous completes,
      rpc controller is overwritten due to the Reset() call - maybe we need to pass the
      pointer to the controller to the callback function also?
    */
    rpcController = new PbRpcController;
    rpcControllerStats = new PbRpcController;
    isGetStatsPending_ = false;
    serviceStub = new OstProto::OstService::Stub(rpcChannel);

    // FIXME(LOW):Can't for my life figure out why this ain't working!
    //QMetaObject::connectSlotsByName(this);
    connect(rpcChannel, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
        this, SLOT(on_rpcChannel_stateChanged()));
    connect(rpcChannel, SIGNAL(connected()), 
        this, SLOT(on_rpcChannel_connected()));
    connect(rpcChannel, SIGNAL(disconnected()),
        this, SLOT(on_rpcChannel_disconnected()));
    connect(rpcChannel, SIGNAL(error(QAbstractSocket::SocketError)), 
        this, SLOT(on_rpcChannel_error(QAbstractSocket::SocketError)));
}

PortGroup::~PortGroup()
{
    qDebug("PortGroup Destructor");
    // Disconnect and free rpc channel etc.
    PortGroup::disconnectFromHost();
    delete serviceStub;
    delete rpcControllerStats;
    delete rpcController;
    delete rpcChannel;
}


// ------------------------------------------------
//                      Slots
// ------------------------------------------------
void PortGroup::on_rpcChannel_stateChanged()
{
    qDebug("state changed");
    emit portGroupDataChanged(mPortGroupId);
}

void PortGroup::on_rpcChannel_connected()
{
    OstProto::Void            void_;
    OstProto::PortIdList    *portIdList;
    
    qDebug("connected\n");
    emit portGroupDataChanged(mPortGroupId);

    qDebug("requesting portlist ...");
    portIdList = new OstProto::PortIdList();
    rpcController->Reset();
    serviceStub->getPortIdList(rpcController, &void_, portIdList, 
        NewCallback(this, &PortGroup::processPortIdList, portIdList));
}

void PortGroup::on_rpcChannel_disconnected()
{
    qDebug("disconnected\n");
    emit portListAboutToBeChanged(mPortGroupId);

    while (!mPorts.isEmpty())
        delete mPorts.takeFirst(); 

    emit portListChanged(mPortGroupId);
    emit portGroupDataChanged(mPortGroupId);
}

void PortGroup::on_rpcChannel_error(QAbstractSocket::SocketError socketError)
{
    qDebug("%s: error %d", __FUNCTION__, socketError);
    emit portGroupDataChanged(mPortGroupId);
}

void PortGroup::when_configApply(int portIndex, uint *cookie)
{
    uint            *op;
    OstProto::Ack    *ack;

    Q_ASSERT(portIndex < mPorts.size());

    if (state() != QAbstractSocket::ConnectedState)
    {
        if (cookie != NULL)
            delete cookie;
        return;
    }

    if (cookie == NULL)
    {
        // cookie[0]: op [0 - delete, 1 - add, 2 - modify, 3 - Done!]
        // cookie[1]: *ack
        cookie = new uint[2];
        ack = new OstProto::Ack;

        cookie[0] = (uint) 0;
        cookie[1] = (uint) ack;
    }
    else
    {
        ack = (OstProto::Ack*) cookie[1];
    }

    Q_ASSERT(cookie != NULL);
    op = &cookie[0];

    switch (*op)
    {
    case 0:
        {
            OstProto::StreamIdList        streamIdList;

            qDebug("applying 'deleted streams' ...");

            streamIdList.mutable_port_id()->set_id(mPorts[portIndex]->id());
            mPorts[portIndex]->getDeletedStreamsSinceLastSync(streamIdList);

            (*op)++;
            rpcController->Reset();
            serviceStub->deleteStream(rpcController, &streamIdList, ack, 
                ::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
            break;
        }

    case 1:
        {
            OstProto::StreamIdList        streamIdList;

            qDebug("applying 'new streams' ...");

            streamIdList.mutable_port_id()->set_id(mPorts[portIndex]->id());
            mPorts[portIndex]->getNewStreamsSinceLastSync(streamIdList);

            (*op)++;
            rpcController->Reset();
            serviceStub->addStream(rpcController, &streamIdList, ack, 
                ::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
            break;
        }

    case 2:
        {
            OstProto::StreamConfigList    streamConfigList;

            qDebug("applying 'modified streams' ...");

            streamConfigList.mutable_port_id()->set_id(mPorts[portIndex]->id());
            mPorts[portIndex]->getModifiedStreamsSinceLastSync(streamConfigList);

            (*op)++;
            rpcController->Reset();
            serviceStub->modifyStream(rpcController, &streamConfigList, ack, 
                ::google::protobuf::NewCallback(this, &PortGroup::when_configApply, portIndex, cookie));
            break;
        }

    case 3:
        qDebug("apply completed");
        mPorts[portIndex]->when_syncComplete();
        delete cookie;
        break;

    default:
        qDebug("%s: Unknown Op!!!", __FUNCTION__);
        break;
    }
}

void PortGroup::processPortIdList(OstProto::PortIdList *portIdList)
{
    qDebug("got a portlist ...");

    if (rpcController->Failed())
    {
        qDebug("%s: rpc failed", __FUNCTION__);
        goto _error_exit;
    }

    emit portListAboutToBeChanged(mPortGroupId);

    for(int i = 0; i <  portIdList->port_id_size(); i++)
    {
        Port *p;
        
        p = new Port(portIdList->port_id(i).id(), mPortGroupId);
        connect(p, SIGNAL(portDataChanged(int, int)), 
                this, SIGNAL(portGroupDataChanged(int, int)));
        qDebug("before port append\n");
        mPorts.append(p);
    }

    emit portListChanged(mPortGroupId);

    this->portIdList.CopyFrom(*portIdList);

    // Request PortConfigList
    {
        OstProto::PortConfigList    *portConfigList;
        
        qDebug("requesting port config list ...");
        portConfigList = new OstProto::PortConfigList();
        rpcController->Reset();
        serviceStub->getPortConfig(rpcController,
            portIdList, portConfigList, NewCallback(this, 
                &PortGroup::processPortConfigList, portConfigList));
    }

    goto _exit;

_error_exit:
_exit:
    delete portIdList;
}

void PortGroup::processPortConfigList(OstProto::PortConfigList *portConfigList)
{
    qDebug("In %s", __FUNCTION__);

    if (rpcController->Failed())
    {
        qDebug("%s: rpc failed", __FUNCTION__);
        goto _error_exit;
    }

    emit portListAboutToBeChanged(mPortGroupId);

    for(int i = 0; i < portConfigList->port_size(); i++)
    {
        uint    id;

        id = portConfigList->port(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updatePortConfig(portConfigList->mutable_port(i));
    }

    emit portListChanged(mPortGroupId);

    // FIXME: check if we need new signals since we are not changing the
    // number of ports, just the port data

    if (numPorts() > 0)
        getStreamIdList();

_error_exit:
    delete portConfigList;
}

void PortGroup::getStreamIdList(int portIndex, 
    OstProto::StreamIdList *streamIdList)
{
    ::OstProto::PortId            portId;

    qDebug("In %s", __FUNCTION__);

    if (streamIdList == NULL)
    {
        // First invocation (uses default params) - 
        //     request StreamIdList for first port

        Q_ASSERT(portIndex == 0);
        Q_ASSERT(numPorts() > 0);
        streamIdList = new ::OstProto::StreamIdList();

        goto _request;
    }

    qDebug("got a streamIdlist ...");

    if (rpcController->Failed())
    {
        qDebug("%s: rpc failed", __FUNCTION__);
        goto _next_port;     // FIXME(MED): Partial RPC
    }

    Q_ASSERT(portIndex < numPorts());

    if (streamIdList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("%s: Invalid portId %d (expected %d) received for portIndex %d",
            __FUNCTION__, streamIdList->port_id().id(), mPorts[portIndex]->id(), 
            portIndex);
        goto _next_port;     // FIXME(MED): Partial RPC
    }

    // FIXME(MED): need to mPorts.clear()???
    for(int i = 0; i < streamIdList->stream_id_size(); i++)
    {
        uint streamId;

        streamId = streamIdList->stream_id(i).id();
        mPorts[portIndex]->insertStream(streamId);
    }

_next_port:
    // FIXME(HI): ideally we shd use signals/slots but this means
    // we will have to use Port* instead of Port with QList<> -
    // need to find a way for this
    mPorts[portIndex]->when_syncComplete();
    portIndex++;
    if (portIndex >= numPorts())
    {
        // We're done for all ports !!!

        // FIXME(HI): some way to reset streammodel 

        delete streamIdList;

        if (numPorts() > 0)
            getStreamConfigList();

        goto _exit;
    }

_request:
    portId.set_id(mPorts[portIndex]->id());
    streamIdList->Clear();

    rpcController->Reset();
    serviceStub->getStreamIdList(rpcController, &portId, streamIdList,
        NewCallback(this, &PortGroup::getStreamIdList,
            portIndex, streamIdList));

    goto _exit;



_exit:
    return;
}

void PortGroup::getStreamConfigList(int portIndex,
    OstProto::StreamConfigList *streamConfigList)
{
    OstProto::StreamIdList    streamIdList;

    qDebug("In %s", __PRETTY_FUNCTION__);

    if (streamConfigList == NULL)
    {
        // First invocation using default params 
        //        - request for first port

        Q_ASSERT(portIndex == 0);
        Q_ASSERT(numPorts() > 0);

        streamConfigList = new OstProto::StreamConfigList;

        goto _request;
    }

    qDebug("got a streamconfiglist");

    if (rpcController->Failed())
    {
        qDebug("%s: rpc failed", __FUNCTION__);
        goto _next_port;
    }

    Q_ASSERT(portIndex < numPorts());

    if (streamConfigList->port_id().id() != mPorts[portIndex]->id())
    {
        qDebug("%s: Invalid portId %d (expected %d) received for portIndex %d",
            __FUNCTION__, streamConfigList->port_id().id(), 
            mPorts[portIndex]->id(), portIndex);
        goto _next_port;     // FIXME(MED): Partial RPC
    }

    // FIXME(MED): need to mStreams.clear()???
    for(int i = 0; i <  streamConfigList->stream_size(); i++)
    {
        uint streamId;

        streamId = streamConfigList->stream(i).stream_id().id();
        mPorts[portIndex]->updateStream(streamId,
            streamConfigList->mutable_stream(i));
    }

_next_port:
    portIndex++;

    if (portIndex >= numPorts())
    {
        // We're done for all ports !!!

        // FIXME(HI): some way to reset streammodel 

        delete streamConfigList;
        goto _exit;
    }

_request:
    qDebug("requesting stream config list ...");

    streamIdList.Clear();
    streamIdList.mutable_port_id()->set_id(mPorts[portIndex]->id());
    for (int j = 0; j < mPorts[portIndex]->numStreams(); j++)
    {
        OstProto::StreamId    *s;

        s = streamIdList.add_stream_id();
        s->set_id(mPorts[portIndex]->streamByIndex(j)->id());
    }
    streamConfigList->Clear();

    rpcController->Reset();
    serviceStub->getStreamConfig(rpcController,
        &streamIdList, streamConfigList, NewCallback(this, 
            &PortGroup::getStreamConfigList, portIndex, streamConfigList));

_exit:
    return;
}

void PortGroup::processModifyStreamAck(OstProto::Ack */*ack*/)
{
    qDebug("In %s", __FUNCTION__);

    qDebug("Modify Successful!!");

    // TODO(HI): Apply Button should now be disabled???!!!!???
}

void PortGroup::startTx(QList<uint> *portList)
{
    OstProto::PortIdList    portIdList;
    OstProto::Ack            *ack;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    ack = new OstProto::Ack;
    if (portList == NULL)
        goto _exit;
    else
    {
        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId    *portId;
            portId = portIdList.add_port_id();
            portId->set_id(portList->at(i));
        }
    }

    serviceStub->startTx(rpcController, &portIdList, ack,
            NewCallback(this, &PortGroup::processStartTxAck, ack));
_exit:
    return;
}

void PortGroup::stopTx(QList<uint> *portList)
{
    OstProto::PortIdList    portIdList;
    OstProto::Ack            *ack;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;

    ack = new OstProto::Ack;

    for (int i = 0; i < portList->size(); i++)
    {
        OstProto::PortId    *portId;
        portId = portIdList.add_port_id();
        portId->set_id(portList->at(i));
    }

    rpcController->Reset();
    serviceStub->stopTx(rpcController, &portIdList, ack,
            NewCallback(this, &PortGroup::processStopTxAck, ack));
_exit:
    return;
}

void PortGroup::startCapture(QList<uint> *portList)
{
    OstProto::PortIdList    portIdList;
    OstProto::Ack            *ack;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;

    ack = new OstProto::Ack;

    for (int i = 0; i < portList->size(); i++)
    {
        OstProto::PortId    *portId;
        portId = portIdList.add_port_id();
        portId->set_id(portList->at(i));
    }

    rpcController->Reset();
    serviceStub->startCapture(rpcController, &portIdList, ack,
            NewCallback(this, &PortGroup::processStartCaptureAck, ack));
_exit:
    return;
}

void PortGroup::stopCapture(QList<uint> *portList)
{
    OstProto::PortIdList    portIdList;
    OstProto::Ack            *ack;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    if ((portList == NULL) || (portList->size() == 0))
        goto _exit;

    ack = new OstProto::Ack;
    for (int i = 0; i < portList->size(); i++)
    {
        OstProto::PortId    *portId;
        portId = portIdList.add_port_id();
        portId->set_id(portList->at(i));
    }

    rpcController->Reset();
    serviceStub->stopCapture(rpcController, &portIdList, ack,
            NewCallback(this, &PortGroup::processStopCaptureAck, ack));
_exit:
    return;
}

void PortGroup::viewCapture(QList<uint> *portList)
{
    static QTemporaryFile    *capFile = NULL;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        goto _exit;

    if ((portList == NULL) || (portList->size() != 1))
        goto _exit;

    if (capFile)
        delete capFile;

    /*! \todo (MED) unable to reuse the same file 'coz capFile->resize(0) is
        not working - it fails everytime */
    capFile = new QTemporaryFile();
    capFile->open();
    qDebug("Temp CapFile = %s", capFile->fileName().toAscii().constData());

    for (int i = 0; i < portList->size(); i++)
    {
        OstProto::PortId    portId;
        OstProto::CaptureBuffer    *buf;

        portId.set_id(portList->at(i));

        buf = new OstProto::CaptureBuffer;
        rpcController->Reset();
        rpcController->setBinaryBlob(capFile);
        serviceStub->getCaptureBuffer(rpcController, &portId, buf,
            NewCallback(this, &PortGroup::processViewCaptureAck, buf, (QFile*) capFile));
    }
_exit:
    return;
}

void PortGroup::processStartTxAck(OstProto::Ack    *ack)
{
    qDebug("In %s", __FUNCTION__);

    delete ack;
}

void PortGroup::processStopTxAck(OstProto::Ack    *ack)
{
    qDebug("In %s", __FUNCTION__);

    delete ack;
}

void PortGroup::processStartCaptureAck(OstProto::Ack    *ack)
{
    qDebug("In %s", __FUNCTION__);

    delete ack;
}

void PortGroup::processStopCaptureAck(OstProto::Ack    *ack)
{
    qDebug("In %s", __FUNCTION__);

    delete ack;
}

void PortGroup::processViewCaptureAck(OstProto::CaptureBuffer *buf, QFile *capFile)
{
#ifdef Q_OS_WIN32
    QString viewer("C:/Program Files/Wireshark/wireshark.exe");
#else
    QString viewer("/usr/bin/wireshark");
#endif

    qDebug("In %s", __FUNCTION__);

    capFile->flush();
    capFile->close();

    if (!QProcess::startDetached(viewer, QStringList() << capFile->fileName()))
        qDebug("Failed starting Wireshark");

    delete buf;
}

void PortGroup::getPortStats()
{
    OstProto::PortStatsList    *portStatsList;

    //qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    if (isGetStatsPending_)
        return;

       portStatsList = new OstProto::PortStatsList;
    rpcControllerStats->Reset();
    isGetStatsPending_ = true;
    serviceStub->getStats(rpcControllerStats, &portIdList, portStatsList,
            NewCallback(this, &PortGroup::processPortStatsList, portStatsList));
}

void PortGroup::processPortStatsList(OstProto::PortStatsList *portStatsList)
{
    //qDebug("In %s", __FUNCTION__);

    if (rpcControllerStats->Failed())
    {
        qDebug("%s: rpc failed", __FUNCTION__);
        goto _error_exit;
    }

    for(int i = 0; i < portStatsList->port_stats_size(); i++)
    {
        uint    id;

        id = portStatsList->port_stats(i).port_id().id();
        // FIXME: don't mix port id & index into mPorts[]
        mPorts[id]->updateStats(portStatsList->mutable_port_stats(i));
    }

    emit statsChanged(mPortGroupId);

_error_exit:
    delete portStatsList;
    isGetStatsPending_ = false;
}

void PortGroup::clearPortStats(QList<uint> *portList)
{
    OstProto::PortIdList    portIdList;
    OstProto::Ack            *ack;

    qDebug("In %s", __FUNCTION__);

    if (state() != QAbstractSocket::ConnectedState)
        return;

    ack = new OstProto::Ack;
    if (portList == NULL)
        portIdList.CopyFrom(this->portIdList);
    else
    {
        for (int i = 0; i < portList->size(); i++)
        {
            OstProto::PortId    *portId;

            portId = portIdList.add_port_id();
            portId->set_id(portList->at(i));
        }
    }

    rpcController->Reset();
    serviceStub->clearStats(rpcController, &portIdList, ack,
            NewCallback(this, &PortGroup::processClearStatsAck, ack));
}

void PortGroup::processClearStatsAck(OstProto::Ack    *ack)
{
    qDebug("In %s", __FUNCTION__);

    // Refresh stats immediately after a stats clear/reset
    getPortStats();

    delete ack;
}

