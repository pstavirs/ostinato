/*
Copyright (C) 2014 Srivats P.

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

#include "samplepdml.h"

#include "sample.pb.h"

/*!
 TODO : Initialize the following inherited protected members -
  - ostProtoId_ 
  - fieldMap_

 ostProtoId_ is the protocol's protobuf field number as defined in
 message 'Protocol' enum 'k' in file protocol.proto

 fieldMap_ is a mapping of the protocol's field names as they appear
 in the PDML to the protobuf field numbers for the protocol. All such
 fields are classified as 'known' fields and the base class will take care
 of decoding these without any help from the subclass. 
 
 Note that the PDML field names are same as the field names used in Wireshark 
 display filters. The full reference for these is available at -
   http://www.wireshark.org/docs/dfref/
*/
PdmlSampleProtocol::PdmlSampleProtocol()
{
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;

    fieldMap_.insert("sample.checksum", OstProto::Sample::kChecksumFieldNumber);
    fieldMap_.insert("sample.x", OstProto::Sample::kXFieldNumber);
    fieldMap_.insert("sample.y", OstProto::Sample::kYFieldNumber);
}

PdmlSampleProtocol::~PdmlSampleProtocol()
{
}

PdmlSampleProtocol* PdmlSampleProtocol::createInstance()
{
    return new PdmlSampleProtocol();
}

/*!
 TODO: Use this method to do any special handling that may be required for
 preprocessing a protocol before parsing/decoding the protocol's fields
*/
void PdmlSampleProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, 
        int /*expectedPos*/, OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return;
}

/*!
 TODO: Use this method to do any special handling or cleanup that may be 
 required when a protocol decode is ending prematurely
*/
void PdmlSampleProtocol::prematureEndHandler(int /*pos*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return;
}

/*!
 TODO: Use this method to do any special handling that may be required for
 postprocessing a protocol after parsing/decoding all the protocol fields

 If your protocol's protobuf has some meta-fields that should be set to
 their non default values, this is a good place to do that. e.g. derived
 fields such as length, checksum etc. may be correct or incorrect in the
 PCAP/PDML - to retain the same value as in the PCAP/PDML and not let
 Ostinato recalculate these, you can set the is_override_length,
 is_override_cksum meta-fields to true here
*/
void PdmlSampleProtocol::postProtocolHandler(OstProto::Protocol* /*pbProto*/,
        OstProto::Stream* /*stream*/)
{
    return;
}

/*!
 TODO: Handle all 'unknown' fields using this method

 You need to typically only handle frame fields or fields actually present
 in the protocol on the wire. So you can safely ignore meta-fields such as
 Good/Bad Checksum. 
 
 Some fields may not have a 'name' attribute, so cannot be classified as 
 a 'known' field. Use this method to identify such fields using other 
 attributes such as 'show' or 'showname' and populate the corresponding 
 protobuf field. 

 If the PDML protocol contains some fields that are not supported by Ostinato,
 use a HexDump protocol as a replacement to store these bytes
*/
void PdmlSampleProtocol::unknownFieldHandler(QString /*name*/, 
        int /*pos*/, int /*size*/, const QXmlStreamAttributes& /*attributes*/, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    return;
}
