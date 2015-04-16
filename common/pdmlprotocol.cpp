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

/*!
  \class PdmlProtocol

  PdmlProtocol is the base class which provides the interface for all
  PDML decode helper protocols

  All Pdml helper classes derived from PdmlProtocol MUST register 
  themselves with PdmlReader. When PdmlReader encounters a 'proto' tag 
  in the PDML during parsing, it instantiates the corresponding helper 
  PdmlProtocol class and calls its methods to decode the protocol.

  A subclass MUST initialize the following inherited protected variables 
  in its constructor -
  - ostProtoId_
  - fieldMap_

  A subclass typically needs to reimplement the following methods -
  - createInstance()

  Depending on certain conditions, subclasses may need to reimplement
  the following additional methods -
  - unknownFieldHandler()
  - preProtocolHandler()
  - postProtocolHandler()

  See the description of the methods for more information.

  Use the SamplePdmlProtocol implementation as boilerplate code and
  for guidelines and tips
*/

/*!
 Constructs the PdmlProtocol
*/
PdmlProtocol::PdmlProtocol()
{
    ostProtoId_ = -1;
}

/*!
 Destroys the PdmlProtocol
*/
PdmlProtocol::~PdmlProtocol()
{
}

/*!
 Allocates and returns a new instance of the class

 Caller is responsible for freeing up after use. Subclasses MUST implement
 this function and register it with PdmlReader
*/
PdmlProtocol* PdmlProtocol::createInstance()
{
    return new PdmlProtocol();
}

/*!
  Returns the protocol's field number as defined in message 'Protocol', enum 'k'
  (file: protocol.proto)
*/
int PdmlProtocol::ostProtoId() const
{
    return ostProtoId_;
}

/*!
 Returns true if name is a 'known' field that can be directly mapped
 to the protobuf field
*/
bool PdmlProtocol::hasField(QString name) const
{
    return fieldMap_.contains(name);
}

/*!
 Returns the protocol's protobuf field number corresponding to name
*/
int PdmlProtocol::fieldId(QString name) const
{
    return fieldMap_.value(name);
}

/*!
 This method is called by PdmlReader before any fields within the protocol
 are processed. All attributes associated with the 'proto' tag in the PDML
 are passed to this method

 Use this method to do any special handling that may be required for
 preprocessing
*/
void PdmlProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, 
        int /*expectedPos*/, OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

/*!
 This method is called by PdmlReader when it encounters a nested
 protocol in the PDML i.e. a protocol within a protocol or a protocol
 within a field

 This is a notification to the protocol that protocol processing will
 be ending prematurely. postProtocolHandler() will still be called in
 such cases.
*/
void PdmlProtocol::prematureEndHandler(int /*pos*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}

/*!
 This method is called by PdmlReader after all fields within the protocol
 are processed.

 Use this method to do any special handling that may be required for
 postprocessing
*/
void PdmlProtocol::postProtocolHandler(OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}


/*!
 This method is called by PdmlReader for each field in the protocol

 Depending on whether it is a known or unknown field, the virtual methods
 knownFieldHandler() and unknownFieldHandler() are invoked
*/
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

/*!
 Handles a 'known' field

 Uses protobuf reflection interface to set the protobuf field name to 
 valueHexStr as per the field's datatype
*/
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

/*!
 Handles a 'unknown' field

 The default implementation does nothing. Subclasses may need to implement
 this if the protocol contains 'unknown' fields.
*/
void PdmlProtocol::unknownFieldHandler(QString /*name*/, 
        int /*pos*/, int /*size*/, const QXmlStreamAttributes& /*attributes*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return; // do nothing!
}
