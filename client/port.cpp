#include <vector>

#include <google/protobuf/descriptor.h>

#include "port.h"
#include "pbhelper.h"


Port::Port(quint32 id, quint32 portGroupId)
{
	d.set_port_id(id);
	mPortGroupId = portGroupId;


#if 0 // PB
	// FIXME(HI): TEST only
	for(int i = 0; i < 10; i++)
		mPortStats[i] = mPortGroupId*10000+mPortId*100+i;
#endif
}

void Port::updatePortConfig(OstProto::PortConfig *portConfig)
{

	PbHelper	pbh;

	pbh.update(&d, portConfig);
#if 0
	const ::google::protobuf::Message::Reflection			*ref1;
	::google::protobuf::Message::Reflection					*ref2;
	std::vector<const ::google::protobuf::FieldDescriptor*>	list;

	qDebug("In %s", __FUNCTION__);

	ref1 = portConfig.GetReflection();
	ref1->ListFields(&list);

	ref2 = d.GetReflection();

	for (uint i=0; i < list.size(); i++)
	{
		const ::google::protobuf::FieldDescriptor *f1, *f2;

		f1 = list[i];
		f2 = d.GetDescriptor()->FindFieldByName(f1->name());
		switch(f2->type())
		{
		case ::google::protobuf::FieldDescriptor::TYPE_UINT32:
			ref2->SetUInt32(f2, ref1->GetUInt32(f1));
			break;
		case ::google::protobuf::FieldDescriptor::TYPE_BOOL:
			ref2->SetBool(f2, ref1->GetBool(f1));
			break;
		case ::google::protobuf::FieldDescriptor::TYPE_STRING:
			ref2->SetString(f2, ref1->GetString(f1));
			break;
		default:
			qDebug("unhandled Field Type");
			break;
		}
	}

	if (msg->GetDescriptor() != OstProto::PortConfig::descriptor())
	{
		qDebug("%s: invalid Message Descriptor (%s)", __FUNCTION__,
			msg->GetDescriptor()->name());
		goto _error_exit;
	}

	portConfig = msg;

	// check for "required" param
	if (!portConfig.has_port_id())
	{
		qDebug("%s: invalid Message Descriptor (%s)", __FUNCTION__,
			msg->GetDescriptor()->name());
		goto _error_exit;
	}
#endif
}

void Port::insertDummyStreams()
{
	mStreams.append(*(new Stream));
	mStreams[0].setName(QString("%1:%2:0").arg(portGroupId()).arg(id()));
#if 1
	mStreams.append(*(new Stream));
	mStreams[1].setName(QString("%1:%2:1").arg(portGroupId()).arg(id()));
#endif
}


