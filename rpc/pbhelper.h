#ifndef _PB_HELPER_H
#define _PB_HELPER_H

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <qDebug.h>

class PbHelper
{
public:
	bool update(
		::google::protobuf::Message *target,
		::google::protobuf::Message *source)
	{
		::google::protobuf::Message::Reflection		*sourceRef;
		::google::protobuf::Message::Reflection		*targetRef;
		std::vector<const ::google::protobuf::FieldDescriptor*>	srcFieldList;

		qDebug("In %s", __FUNCTION__);

		if (source->GetDescriptor()->full_name() !=
			target->GetDescriptor()->full_name())
			goto _error_exit;

		sourceRef = source->GetReflection();
		targetRef = target->GetReflection();

		sourceRef->ListFields(&srcFieldList);
		for (uint i=0; i < srcFieldList.size(); i++)
		{
			const ::google::protobuf::FieldDescriptor *srcField, *targetField;

			srcField = srcFieldList[i];
			targetField = target->GetDescriptor()->FindFieldByName(
				srcField->name());

			switch(targetField->type())
			{
			case ::google::protobuf::FieldDescriptor::TYPE_UINT32:
				targetRef->SetUInt32(targetField, 
					sourceRef->GetUInt32(srcField));
				break;
			case ::google::protobuf::FieldDescriptor::TYPE_BOOL:
				targetRef->SetBool(targetField,
					sourceRef->GetBool(srcField));
				break;
			case ::google::protobuf::FieldDescriptor::TYPE_STRING:
				targetRef->SetString(targetField,
					sourceRef->GetString(srcField));
				break;
			default:
				qDebug("unhandled Field Type");
				break;
			}
		}

		return true;

	_error_exit:
		qDebug("%s: error!", __FUNCTION__);
		return false;
	}

};
#endif
