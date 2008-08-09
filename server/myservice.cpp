#include "myservice.h"
#include "qdebug.h"

#define LOG(...)   {sprintf(logStr, __VA_ARGS__); host->Log(logStr);}

MyService::MyService(AbstractHost *host)
{
    pcap_if_t *dev;
    int i=0;
    char errbuf[PCAP_ERRBUF_SIZE];

	// Init Data
	this->host = host;
	numPorts = 0;
	alldevs = NULL;

    LOG("Retrieving the device list from the local machine\n"); 
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
    {
        LOG("Error in pcap_findalldevs_ex: %s\n", errbuf);
        goto _fail;
    }

	/* Count number of local ports */
    for(dev = alldevs; dev != NULL; dev = dev->next)
		numPorts++;
   
   	portInfo = new PortInfo[numPorts];

    /* Populate and Print the list */
    for(i=0, dev=alldevs; dev!=NULL; i++, dev=dev->next)
    {
#if 0 // PB
		//portInfo[i].portId = i;
		//portInfo[i].dev = dev;
		//portInfo[i].streamHead = NULL;
		//portInfo[i].streamTail = NULL;
#endif
		portInfo[i].setId(i);
		portInfo[i].setPcapDev(dev);
#if 1
        LOG("%d. %s", i, dev->name);
        if (dev->description)
		{
            LOG(" (%s)\n", dev->description);
		}
        else
            LOG(" (No description available)\n");
#endif
    }
    
    if (i == 0)
    {
        LOG("\nNo interfaces found! Make sure WinPcap is installed.\n");
        goto _fail;
    }

_fail:
	return;
}

MyService::~MyService()
{
	unsigned int i;
#if 0 // PB?????
	for (i = 0; i < numPorts; i++)
		DeleteAllStreams(i);
#endif
    pcap_freealldevs(alldevs);
}

void MyService::getPortIdList(
	::google::protobuf::RpcController* controller,
	const ::OstProto::Void* request,
	::OstProto::PortIdList* response,
	::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);

	for (uint i = 0; i < numPorts; i++)
		response->add_port_id(portInfo[i].d.port_id());

	qDebug("Server(%s): portid count = %d", __FUNCTION__, response->port_id_size());

	qDebug("Server(%s): %s", __FUNCTION__, response->DebugString().c_str());

	done->Run();
}

void MyService::getPortConfig(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::PortConfigList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);

	for (int i=0; i < request->port_id_size(); i++)
	{
		unsigned int id;

		id = request->port_id(i);
		if (id < numPorts)
		{
			OstProto::PortConfig	*p;

			p = response->add_list();
			p->CopyFrom(portInfo[request->port_id(i)].d);
		}
	}

	done->Run();
}

void MyService::getStreamIdList(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::StreamIdList* response,
::google::protobuf::Closure* done)
{

	for (int i = 0; i < request->port_id_size(); i++)
	{
		unsigned int portId;

		portId = request->port_id(i);
		if (portId >= numPorts)
		{
			qDebug("%s: Invalid port id %d", __FUNCTION__, portId);
			continue;		// TODO: Partial status of RPC
		}

		for (int j = 0; j < portInfo[portId].streamList.size(); j++)
		{
			OstProto::StreamId	*s, *q;

			q = portInfo[portId].streamList[j].d.mutable_id();
			assert(q->port_id() == portId);

			s = response->add_id();
			s->set_port_id(portId);
			s->set_stream_id(q->stream_id());
		}
	}
	done->Run();
}

void MyService::getStreamConfig(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::StreamConfigList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);

	for (int i = 0; i < request->id_size(); i++)
	{
		unsigned int portId;
		unsigned int streamId;

		portId = request->id(i).port_id();
		if (portId >= numPorts)
		{
			qDebug("%s: Invalid port id %d", __FUNCTION__, portId);
			continue;		// TODO: Partial status of RPC
		}

		streamId = request->id(i).stream_id();
		if (streamId >= numPorts)
		{
			qDebug("%s: Invalid port id %d", __FUNCTION__, portId);
			continue;		// TODO: Partial status of RPC
		}

		for (int j = 0; j < portInfo[portId].streamList.size(); j++)
		{
			OstProto::Stream	*s, *q;

#if 0
			q = portInfo[portId].streamList[j].d.e_stream();
			assert(q->port_id() == portId);

			s = response->add_stream();
			s->set_port_id(portId);
			s->set_stream_id(q->stream_id());
#endif
			// TODO: more params
		}
	}
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::addStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::deleteStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::modifyStream(::google::protobuf::RpcController* controller,
const ::OstProto::StreamConfigList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::startTx(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::stopTx(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::startCapture(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::stopCapture(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::getCaptureBuffer(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::CaptureBufferList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::getStats(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::PortStatsList* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

void MyService::clearStats(::google::protobuf::RpcController* controller,
const ::OstProto::PortIdList* request,
::OstProto::Ack* response,
::google::protobuf::Closure* done)
{
	qDebug("In %s", __FUNCTION__);
	controller->SetFailed("Not Implemented");
	done->Run();
}

