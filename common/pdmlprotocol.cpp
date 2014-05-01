/*
Copyright (C) 2011 Srivats P.

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

#include "pdmlprotocol.h"

PdmlProtocol::PdmlProtocol()
{
    ostProtoId_ = -1;
}

PdmlProtocol::~PdmlProtocol()
{
}

PdmlProtocol* PdmlProtocol::createInstance()
{
    return new PdmlProtocol();
}

int PdmlProtocol::ostProtoId() const
{
    return ostProtoId_;
}

bool PdmlProtocol::hasField(QString name) const
{
    return fieldMap_.contains(name);
}

int PdmlProtocol::fieldId(QString name) const
{
    return fieldMap_.value(name);
}

void PdmlProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, 
        int /*expectedPos*/, OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlProtocol::prematureEndHandler(int /*pos*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlProtocol::postProtocolHandler(OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

void PdmlProtocol::fieldHandler(QString name, 
        const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (hasField(name))
    {
        QString valueHexStr = attributes.value("value").toString();

        qDebug("\t(KNOWN) fieldName:%s, value:%s",
                name.toAscii().constData(), 
                valueHexStr.toAscii().constData());

        knownFieldHandler(name, valueHexStr, pbProto);
    }
    else
    {
        int pos = -1;
        int size = -1;

        if (!attributes.value("pos").isEmpty())
            pos = attributes.value("pos").toString().toInt();
        if (!attributes.value("size").isEmpty())
            size = attributes.value("size").toString().toInt();

        qDebug("\t(UNKNOWN) fieldName:%s, pos:%d, size:%d",
                name.toAscii().constData(), pos, size);

        unknownFieldHandler(name, pos, size, attributes, pbProto, stream);
    }
}

void PdmlProtocol::knownFieldHandler(QString name, QString valueHexStr,
        OstProto::Protocol *pbProto)
{
    const google::protobuf::Reflection *protoRefl = pbProto->GetReflection();
    const google::protobuf::FieldDescriptor *extDesc = 
                protoRefl->FindKnownExtensionByNumber(ostProtoId());

    google::protobuf::Message *msg = 
                protoRefl->MutableMessage(pbProto,extDesc);

    const google::protobuf::Reflection *msgRefl = msg->GetReflection();
    const google::protobuf::FieldDescriptor *fieldDesc = 
                msg->GetDescriptor()->FindFieldByNumber(fieldId(name));

    bool isOk;

    Q_ASSERT(fieldDesc != NULL);
    switch(fieldDesc->cpp_type())
    {
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        msgRefl->SetBool(msg, fieldDesc, bool(valueHexStr.toUInt(&isOk)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // TODO
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        msgRefl->SetUInt32(msg, fieldDesc, 
                valueHexStr.toUInt(&isOk, kBaseHex));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        msgRefl->SetUInt64(msg, fieldDesc, 
                valueHexStr.toULongLong(&isOk, kBaseHex));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    {
        QByteArray hexVal = QByteArray::fromHex(valueHexStr.toUtf8());
        std::string str(hexVal.constData(), hexVal.size());
        msgRefl->SetString(msg, fieldDesc, str);
        break;
    }
    default:
        qDebug("%s: unhandled cpptype = %d", __FUNCTION__, 
                fieldDesc->cpp_type());
    }
}

void PdmlProtocol::unknownFieldHandler(QString /*name*/, 
        int /*pos*/, int /*size*/, const QXmlStreamAttributes& /*attributes*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}
