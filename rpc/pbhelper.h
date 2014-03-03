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

#ifndef _PB_HELPER_H
#define _PB_HELPER_H

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <qdebug.h>

#if 0 // not reqd. any longer?
class PbHelper
{
public:

    // FIXME: Change msg from * to &
    void ForceSetSingularDefault(::google::protobuf::Message *msg)
    {
        const ::google::protobuf::Descriptor        *desc;
        ::google::protobuf::Message::Reflection        *refl;

        qDebug("In %s", __FUNCTION__);

        desc = msg->GetDescriptor();
        refl = msg->GetReflection();

        for (int i=0; i < desc->field_count(); i++)
        {
            const ::google::protobuf::FieldDescriptor *f;

            f = desc->field(i);

            // Ensure field is singular and not already set
            if (f->label() == 
                ::google::protobuf::FieldDescriptor::LABEL_REPEATED)
                continue;
            if (refl->HasField(f))
                continue;

            switch(f->type())
            {
            case ::google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                refl->SetDouble(f, refl->GetDouble(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_FLOAT:
                refl->SetFloat(f, refl->GetFloat(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_INT32:
            case ::google::protobuf::FieldDescriptor::TYPE_SINT32:
            case ::google::protobuf::FieldDescriptor::TYPE_SFIXED32:
                refl->SetInt32(f, refl->GetInt32(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_INT64:
            case ::google::protobuf::FieldDescriptor::TYPE_SINT64:
            case ::google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                refl->SetInt64(f, refl->GetInt64(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_UINT32:
            case ::google::protobuf::FieldDescriptor::TYPE_FIXED32:
                refl->SetUInt32(f, refl->GetUInt32(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_UINT64:
            case ::google::protobuf::FieldDescriptor::TYPE_FIXED64:
                refl->SetUInt64(f, refl->GetUInt64(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_BOOL:
                refl->SetBool(f, refl->GetBool(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_ENUM:
                refl->SetEnum(f, refl->GetEnum(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_STRING:
            case ::google::protobuf::FieldDescriptor::TYPE_BYTES:
                refl->SetString(f, refl->GetString(f));
                break;

            case ::google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            case ::google::protobuf::FieldDescriptor::TYPE_GROUP:
                ForceSetSingularDefault(refl->MutableMessage(f)); // recursion!
                break;

            default:
                qDebug("unhandled Field Type");
                break;
            }
        }    
    }

    bool update(
        ::google::protobuf::Message *target,
        ::google::protobuf::Message *source)
    {
        // FIXME(HI): Depracate: use MergeFrom() directly
        qDebug("In %s", __FUNCTION__);
        target->MergeFrom(*source);
        return true;
#if 0
        ::google::protobuf::Message::Reflection        *sourceRef;
        ::google::protobuf::Message::Reflection        *targetRef;
        std::vector<const ::google::protobuf::FieldDescriptor*>    srcFieldList;


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
    _error_exit:
        qDebug("%s: error!", __FUNCTION__);
        return false;
#endif
    }
};
#endif
#endif
