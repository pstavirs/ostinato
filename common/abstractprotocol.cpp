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

#include "abstractprotocol.h" 

#include "protocollistiterator.h"
#include "streambase.h"

#include "bswap.h"

#include <qendian.h>

#if 0
#ifdef qDebug
#undef qDebug
#define qDebug(...)
#endif
#endif

/*!
  \class AbstractProtocol

  AbstractProtocol is the base abstract class which provides the interface
  for all protocols.

  All protocols supported by Ostinato are derived from AbstractProtocol. Apart
  from defining the interface for a protocol, it also provides sensible default
  implementations for methods so that the subclasses need not re-implement. It
  also provides convenience functions for subclasses to use such as methods to
  retrieve payload size, checksum etc.
  
  A subclass typically needs to reimplement the following methods -
  - name()
  - shortName()
  - createInstance()
  - protocolNumber()
  - protoDataCopyInto() [pure virtual]
  - protoDataCopyFrom() [pure virtual]
  - fieldCount()
  - fieldFlags()
  - fieldData()
  - setFieldData()

  Depending on certain conditions, subclasses may need to reimplement the
  following additional methods -
  - protocolIdType()
  - protocolId()
  - protocolFrameSize()
  - isProtocolFrameSizeVariable()
  - protocolFrameVariableCount()

  See the description of the methods for more information.

  Most of the above methods just need some standard boilerplate code - 
  the SampleProtocol implementation includes the boilerplate
*/

/*!
  Constructs an abstract protocol for the given stream and parent

  parent is typically NULL except for protocols which are part of a 
  ComboProtocol
*/
AbstractProtocol::AbstractProtocol(StreamBase *stream, AbstractProtocol *parent)
{
    //qDebug("%s: &prev = %p &next = %p", __FUNCTION__, &prev, &next);
    mpStream = stream;
    this->parent = parent;
    prev = next = NULL;
    _metaFieldCount = -1;
    _frameFieldCount = -1;
    _frameVariableCount = -1;
    protoSize = -1;
    _hasPayload = true;
    _cacheFlags |= FieldFrameBitOffsetCache;
}

/*!
  Destroys the abstract protocol
*/
AbstractProtocol::~AbstractProtocol()
{
}

/*! 
  Allocates and returns a new instance of the class. 
  
  Caller is responsible for freeing up after use.  Subclasses MUST implement 
  this function 
*/
AbstractProtocol* AbstractProtocol::createInstance(StreamBase* /* stream */,
    AbstractProtocol* /* parent */)
{
    return NULL;
}

/*!
  Returns the protocol's field number as defined in message 'Protocol', enum 'k'
  (file: protocol.proto)

  Subclasses MUST implement this function

  \todo convert this to a protected data member instead of a virtual function
*/
quint32 AbstractProtocol::protocolNumber() const
{
    qFatal("Something wrong!!!");
    return 0xFFFFFFFF;
}

/*!
 Copies the common data (not specific to individual protocols) in the 
 protocol member protobuf data into the passed in protocol parameter.
 The individual protocol specific protobuf data is copied using 
 protoDataCopyInto()
*/
void AbstractProtocol::commonProtoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.clear_variable_field();
    for (int i = 0; i < _data.variable_field_size(); i++)
    {
        OstProto::VariableField *vf;

        vf = protocol.add_variable_field();
        vf->CopyFrom(_data.variable_field(i));
    }
}

/*!
 Copies the common data (not specific to individual protocols) from the 
 passed in param protocol protobuf into the member protobuf data.
 The individual protocol specific protobuf data is copied using 
 protoDataCopyFrom()
*/
void AbstractProtocol::commonProtoDataCopyFrom(const OstProto::Protocol &protocol)
{
    _data.clear_variable_field();
    for (int i = 0; i < protocol.variable_field_size(); i++)
    {
        OstProto::VariableField *vf;

        vf = _data.add_variable_field();
        vf->CopyFrom(protocol.variable_field(i));
    }
}

/*!
  \fn virtual void AbstractProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const = 0

  Copy the protocol's protobuf as an extension into the passed in protocol

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/


/*!
  \fn virtual void AbstractProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol) = 0

  Copy and update the protocol's protobuf member data variable from the 
  passed in protocol

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/


/*!
  Returns the full name of the protocol

  The default implementation returns a null string 
*/
QString AbstractProtocol::name() const
{
    return QString(); 
}

/*!
  Returns the short name or abbreviation of the protocol

  The default implementation forms and returns an abbreviation composed
  of all the upper case chars in name() \n 
  The default implementation caches the abbreviation on its first invocation
  and subsequently returns the cached abbreviation 
*/
QString AbstractProtocol::shortName() const
{
    if (protoAbbr.isNull())
    {
        QString abbr;

        for (int i = 0; i < name().size(); i++)
            if (name().at(i).isUpper()) abbr.append(name().at(i));

        if (abbr.size())
            protoAbbr = abbr;
        else
            protoAbbr = QString("");
    }

    return protoAbbr;
}

/*! 
  Returns the number of fields in the protocol (both Frame fields and 
  Meta fields)

  The default implementation returns zero. Subclasses MUST implement this
  function.
*/
int AbstractProtocol::fieldCount() const
{
    return 0;
}

/*! 
  Returns the number of meta fields 

  The default implementation counts and returns the number of fields for which
  the MetaField flag is set\n
  The default implementation caches the count on its first invocation
  and subsequently returns the cached count 
*/
int AbstractProtocol::metaFieldCount() const
{
    if (_metaFieldCount < 0)
    {
        int c = 0;
        for (int i = 0; i < fieldCount() ; i++) 
            if (fieldFlags(i).testFlag(MetaField))
                c++;
        _metaFieldCount = c;
    }

    return _metaFieldCount;
}

/*! 
  Returns the number of frame fields

  The default implementation counts and returns the number of fields for which
  the FrameField flag is set\n
  The default implementation caches the count on its first invocation
  and subsequently returns the cached count

  Subclasses which export different sets of fields based on a opcode/type
  (e.g. icmp) should re-implement this function
*/
int AbstractProtocol::frameFieldCount() const
{
    if (_frameFieldCount < 0)
    {
        int c = 0;
        for (int i = 0; i < fieldCount() ; i++) 
            if (fieldFlags(i).testFlag(FrameField))
                c++;
        _frameFieldCount = c;
    }

    return _frameFieldCount;
}

/*!
  Returns the field flags for the passed in field index

  The default implementation assumes all fields to be frame fields and returns
  'FrameField'. Subclasses must reimplement this method if they have any
  meta fields or checksum fields. See the SampleProtocol for an example.
*/
AbstractProtocol::FieldFlags AbstractProtocol::fieldFlags(int /*index*/) const
{
    return FrameField;
}

/*! 
  Returns the requested field attribute data

  Protocols which have meta fields that vary a frame field across
  streams may use the streamIndex to return the appropriate field value \n
  Some field attributes e.g. FieldName may be invariant across streams\n
  The FieldTextValue attribute may include additional information about
  the field's value e.g. a checksum field may include "(correct)" or 
  "(incorrect)" alongwith the actual checksum value. \n

  The default implementation returns a empty string for FieldName and 
  FieldTextValue; empty byte array of size 0 for FieldFrameValue; 0 for
  FieldValue; subclasses are expected to return meaning values for all
  these attributes. The only exception is the 'FieldBitSize' attribute - 
  the default implementation takes the (byte) size of FieldFrameValue,
  multiplies it with 8 and returns the result - this can be used by
  subclasses for fields which are an integral multiple of bytes; for
  fields whose size are a non-integral multiple of bytes or smaller than
  a byte, subclasses should return the correct value. Also for fields
  which represent checksums, subclasses should return a value for
  FieldBitSize - even if it is an integral multiple of bytes.

  \note If a subclass uses any of the below functions to derive 
  FieldFrameValue, the subclass should handle and return a value for 
  FieldBitSize to prevent endless recursion -
  - protocolFrameCksum()
  - protocolFramePayloadSize()
*/
QVariant AbstractProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (attrib)
    {
        case FieldName:
            return QString();
        case FieldBitSize:
            Q_ASSERT_X(!fieldFlags(index).testFlag(CksumField),
                "AbstractProtocol::fieldData()",
                "FieldBitSize for checksum fields need to be handled by the subclass");
            return fieldData(index, FieldFrameValue, streamIndex).
                toByteArray().size() * 8;
        case FieldValue:
            return 0;
        case FieldFrameValue:
            return QByteArray();
        case FieldTextValue:
            return QString();

        default:
            qFatal("%s:%d: unhandled case %d\n", __FUNCTION__, __LINE__,
                    attrib);
    }

    return QVariant();
}

/*! 
  Sets the value of a field corresponding to index

  This method is called by the GUI code to store a user specified value into
  the protocol's protoBuf.  Currently this method is called with 
  FieldAttrib = FieldValue only.

  Returns true if field is successfully set, false otherwise.
  The default implementation always returns false. Subclasses should 
  reimplement this method. See SampleProtocol for an example.

*/
bool AbstractProtocol::setFieldData(int /*index*/, const QVariant& /*value*/,
        FieldAttrib /*attrib*/)
{
    return false;
}

/*!
 * Returns the bit offset where the specified field starts within the 
 * protocolFrameValue()
 */
int AbstractProtocol::fieldFrameBitOffset(int index, int streamIndex) const
{
    int ofs = 0;

    if ((index < 0) || (index >= fieldCount()) 
            || !fieldFlags(index).testFlag(FrameField))
        return -1;

    // Lookup Cache; if not available calculate and cache (if enabled)

    if (_fieldFrameBitOffset.contains(index)) {
        ofs = _fieldFrameBitOffset.value(index);
        goto _exit;
    }

    for (int i = 0; i < index; i++) {
        if ((_cacheFlags & FieldFrameBitOffsetCache)
                && !_fieldFrameBitOffset.contains(i))
            _fieldFrameBitOffset.insert(i, ofs);
        ofs += fieldData(i, FieldBitSize, streamIndex).toInt();
    }
    if ((_cacheFlags & FieldFrameBitOffsetCache))
        _fieldFrameBitOffset.insert(index, ofs);

    qDebug("======> ffbo index: %d, ofs: %d", index, ofs);
_exit:
    return ofs;
}

/*!
 * Returns the count of variableFields in the protocol
 */
int AbstractProtocol::variableFieldCount() const
{
    return _data.variable_field_size();
}

/*!
 * Appends a variableField to the protocol
 */
void AbstractProtocol::appendVariableField(const OstProto::VariableField &vf)
{
    _data.add_variable_field()->CopyFrom(vf);

    // Update the cached value
    _frameVariableCount = lcm(_frameVariableCount, vf.count());
}

/*!
 * Removes the variableField from the protocol at the specified index
 */
void AbstractProtocol::removeVariableField(int index)
{
    OstProto::Protocol temp;

    if (index >= _data.variable_field_size()) {
        qWarning("%s: %s variableField[%d] out of range; count: %d)",
                __FUNCTION__, qPrintable(shortName()), 
                index, _data.variable_field_size());
        return;
    }

    // TODO: this is inefficient - evaluate using RepeatedPtrField?
    for (int i = 0; i < _data.variable_field_size(); i++) {
        if (i == index)
            continue;
        temp.add_variable_field()->CopyFrom(_data.variable_field(i));
    }

    _data.clear_variable_field();
    _frameVariableCount = 1;
    for (int i = 0; i < temp.variable_field_size(); i++) {
        _data.add_variable_field()->CopyFrom(temp.variable_field(i));
        // Recalculate the cached value
        _frameVariableCount = lcm(_frameVariableCount,
                                  _data.variable_field(i).count());
    }
}

/*!
 * Returns the variableField at the specified index as a constant Reference
 * i.e. read-only
 */
const OstProto::VariableField& AbstractProtocol::variableField(int index) const
{
    Q_ASSERT(index < _data.variable_field_size());

    return _data.variable_field(index);
}

/*!
 * Returns the variableField at the specified index as a mutable pointer.
 * Changes made via the pointer will be reflected in the protocol
 */
OstProto::VariableField* AbstractProtocol::mutableVariableField(int index)
{
    if ((index < 0) || (index >= _data.variable_field_size()))
        return NULL;

    // Invalidate the cached value as the caller may potentially modify it
    _frameVariableCount = -1;

    return _data.mutable_variable_field(index);
}

/*!
  Returns the protocolIdType for the protocol

  The default implementation returns ProtocolIdNone. If a subclass has a
  protocolId field it should return the appropriate value e.g. IP protocol
  will return ProtocolIdIp, Ethernet will return ProtocolIdEth etc.
*/
AbstractProtocol::ProtocolIdType AbstractProtocol::protocolIdType() const
{
    return ProtocolIdNone;
}

/*!
  Returns the protocol id of the protocol for the given type

  The default implementation returns 0. If a subclass represents a protocol
  which has a particular protocol id, it should return the appropriate value.
  If a protocol does not have an id for the given type, it should defer to
  the base class. e.g. IGMP will return 2 for ProtocolIdIp, and defer to the
  base class for the remaining ProtocolIdTypes; IP will return 0x800 for 
  ProtocolIdEth type, 0x060603 for ProtocolIdLlc and 0x04 for ProtocolIdIp etc.
*/
quint32 AbstractProtocol::protocolId(ProtocolIdType /*type*/) const
{
    return 0;
}

/*!
  Returns the protocol id of the payload protocol (the protocol that 
  immediately follows the current one)

  A subclass which has a protocol id field, can use this to retrieve the
  appropriate value
*/
quint32 AbstractProtocol::payloadProtocolId(ProtocolIdType type) const
{
    quint32 id;

    if (next)
        id = next->protocolId(type);
    else if (parent)
        id = parent->payloadProtocolId(type);
    else
        id = 0xFFFFFFFF;

    qDebug("%s: payloadProtocolId = 0x%x", __FUNCTION__, id);
    return id;
}

/*!
  Returns the protocol's size in bytes

  The default implementation sums up the individual field bit sizes and
  returns it. The default implementation calculates the caches the size on
  the first invocation and subsequently returns the cached size.

  If the subclass protocol has a varying protocol size, it MUST reimplement
  this method, otherwise the default implementation is sufficient.
*/
int AbstractProtocol::protocolFrameSize(int streamIndex) const
{
    if (protoSize < 0)
    {
        int bitsize = 0;

        for (int i = 0; i < fieldCount(); i++)
        {
            if (fieldFlags(i).testFlag(FrameField))
                bitsize += fieldData(i, FieldBitSize, streamIndex).toUInt();
        }
        protoSize = (bitsize+7)/8;
    }

    qDebug("%s: protoSize = %d", __FUNCTION__, protoSize);
    return protoSize;
}

/*!
  Returns the byte offset in the packet where the protocol starts

  This method is useful only for "padding" protocols i.e. protocols which
  fill up the remaining space for the user defined packet size e.g. the 
  PatternPayload protocol
*/
int AbstractProtocol::protocolFrameOffset(int streamIndex) const
{
    int size = 0;
    AbstractProtocol *p = prev;
    while (p)
    {
        size += p->protocolFrameSize(streamIndex);
        p = p->prev;
    }

    if (parent)
        size += parent->protocolFrameOffset(streamIndex);

    qDebug("%s: ofs = %d", __FUNCTION__, size);
    return size;
}

/*!
  Returns the size of the payload in bytes. The payload includes all protocols
  subsequent to the current

  This method is useful for protocols which need to fill in a payload size field
*/
int AbstractProtocol::protocolFramePayloadSize(int streamIndex) const
{
    int size = 0;
    AbstractProtocol *p = next;
    while (p)
    {
        size += p->protocolFrameSize(streamIndex);
        p = p->next;
    }
    if (parent)
        size += parent->protocolFramePayloadSize(streamIndex);

    qDebug("%s: payloadSize = %d", __FUNCTION__, size);
    return size;
}


/*! 
  Returns a byte array encoding the protocol (and its fields) which can be
  inserted into the stream's frame

  The default implementation forms and returns an ordered concatenation of 
  the FrameValue of all the 'frame' fields of the protocol also taking care of 
  fields which are not an integral number of bytes\n 
*/
QByteArray AbstractProtocol::protocolFrameValue(int streamIndex, bool forCksum,
        FrameValueAttrib */*attrib*/) const
{
    QByteArray proto, field;
    uint bits, lastbitpos = 0;
    FieldFlags flags;

    for (int i=0; i < fieldCount() ; i++) 
    {
        flags = fieldFlags(i);
        if (flags.testFlag(FrameField))
        {
            bits = fieldData(i, FieldBitSize, streamIndex).toUInt();
            if (bits == 0)
                continue;
            Q_ASSERT(bits > 0);

            if (forCksum && flags.testFlag(CksumField))
            {
                field.resize((bits+7)/8);
                field.fill('\0');
            }
            else
                field = fieldData(i, FieldFrameValue, streamIndex).toByteArray();
            qDebug("<<< (%d, %db) %s >>>", proto.size(), lastbitpos,
                    qPrintable(QString(proto.toHex())));
            qDebug("  < %d: (%db/%dB) %s >", i, bits, field.size(),
                    qPrintable(QString(field.toHex())));

            if (bits == (uint) field.size() * 8)
            {
                if (lastbitpos == 0)
                    proto.append(field);
                else
                {
                    Q_ASSERT(field.size() > 0);

                    char c = proto[proto.size() - 1];
                    proto[proto.size() - 1] =  
                        c | ((uchar)field.at(0) >> lastbitpos);
                    for (int j = 0; j < field.size() - 1; j++)
                        proto.append(field.at(j) << lastbitpos |
                                (uchar)field.at(j+1) >> lastbitpos);
                    proto.append(field.at(field.size() - 1) << lastbitpos);
                }
            }
            else if (bits < (uint) field.size() * 8)
            {
                uchar c;
                uint v;

                v = (field.size()*8) - bits;

                Q_ASSERT(v < 8);

                if (lastbitpos == 0)
                {
                    for (int j = 0; j < field.size(); j++)
                    {
                        c = field.at(j) << v;
                        if ((j+1) < field.size())
                            c |= ((uchar)field.at(j+1) >> (8-v));
                        proto.append(c);
                    }

                    lastbitpos = (lastbitpos + bits) % 8;
                }
                else
                {
                    Q_ASSERT(proto.size() > 0);

                    for (int j = 0; j < field.size(); j++)
                    {
                        uchar d;

                        c = field.at(j) << v;
                        if ((j+1) < field.size())
                            c |= ((uchar) field.at(j+1) >> (8-v));
                        d = proto[proto.size() - 1];
                        proto[proto.size() - 1] = d | ((uchar) c >> lastbitpos);
                        if (bits > (8*j + (8 - v)))
                            proto.append(c << (8-lastbitpos));
                    }

                    lastbitpos = (lastbitpos + bits) % 8;
                }
            }
            else // if (bits > field.size() * 8)
            {
                qFatal("bitsize more than FrameValue size. skipping...");
                continue;
            }
        }
    }

    // Overwrite proto with the variable fields, if any
    for (int i = 0; i < _data.variable_field_size(); i++)
    {
        OstProto::VariableField vf = _data.variable_field(i);
        varyProtocolFrameValue(proto, streamIndex, vf);
    }

    return proto;
}

/*!
  Returns true if the protocol varies one or more of its fields at run-time,
  false otherwise
*/
bool AbstractProtocol::isProtocolFrameValueVariable() const
{
    return (protocolFrameVariableCount() > 1);
}

/*!
  Returns true if the protocol varies its size at run-time, false otherwise

  The default implmentation returns false. A subclass should reimplement
  if it varies its size at run-time e.g. a Payload protocol for a stream with
  incrementing/decrementing frame lengths
*/
bool AbstractProtocol::isProtocolFrameSizeVariable() const
{
    return false;
}

/*!
  Returns the minimum number of frames required for the protocol to 
  vary its fields

  This is the lowest common multiple (LCM) of the counts of all the varying 
  fields in the protocol. Use the AbstractProtocol::lcm() static utility
  function to calculate the LCM.

  The default implementation returns the LCM of all variableFields
  A subclass should reimplement if it has varying fields e.g. an IP protocol 
  that increments/decrements the IP address with every packet.\n
  Subclasses should call the base class method to retreive the count and
  do a LCM with the subclass' own varying fields

*/
int AbstractProtocol::protocolFrameVariableCount() const
{
    if (_frameVariableCount > 0)
        return _frameVariableCount;

    _frameVariableCount = 1;
    for (int i = 0; i < _data.variable_field_size(); i++)
        _frameVariableCount = lcm(_frameVariableCount, 
                                  _data.variable_field(i).count());

    return _frameVariableCount;
}

/*!
  Returns true if the payload content for a protocol varies at run-time,
  false otherwise

  This is useful for subclasses which have fields dependent on payload content 
  (e.g. UDP has a checksum field that varies if the payload varies)
*/
bool AbstractProtocol::isProtocolFramePayloadValueVariable() const
{
    // TODO: it is simpler to do the following -
    //     return (protocolFramePayloadVariableCount() > 1)
    // However, it may be inefficient till the time we cache the 
    // variable count
    AbstractProtocol *p = next;

    while (p)
    {
        if (p->isProtocolFrameValueVariable())
            return true;
        p = p->next;
    }
    if (parent && parent->isProtocolFramePayloadValueVariable())
        return true;

    return false;
}

/*!
  Returns true if the payload size for a protocol varies at run-time,
  false otherwise

  This is useful for subclasses which have fields dependent on payload size 
  (e.g. UDP has a checksum field that varies if the payload varies)
*/
bool AbstractProtocol::isProtocolFramePayloadSizeVariable() const
{
    AbstractProtocol *p = next;

    while (p)
    {
        if (p->isProtocolFrameSizeVariable())
            return true;
        p = p->next;
    }
    if (parent && parent->isProtocolFramePayloadSizeVariable())
        return true;

    return false;
}

/*!
  Returns true if the payload size for a protocol varies at run-time,
  false otherwise

  This is useful for subclasses which have fields dependent on payload size 
  (e.g. UDP has a checksum field that varies if the payload varies)
*/
int AbstractProtocol::protocolFramePayloadVariableCount() const
{
    int count = 1;
    AbstractProtocol *p = next;

    while (p)
    {
        if (p->isProtocolFrameValueVariable() 
                || p->isProtocolFrameSizeVariable())
            count = lcm(count, p->protocolFrameVariableCount());
        p = p->next;
    }
    if (parent && (parent->isProtocolFramePayloadValueVariable()
                    || parent->isProtocolFramePayloadSizeVariable()))
        count = lcm(count, parent->protocolFramePayloadVariableCount());

    return false;
}

/*!
  Returns true if the protocol typically contains a payload or other protocols
  following it e.g. TCP, UDP have payloads, while ARP, IGMP do not 

  The default implementation returns true. If a subclass does not have a
  payload, it should set the _hasPayload data member to false
*/
bool AbstractProtocol::protocolHasPayload() const
{
    return _hasPayload;
}

/*!
  Returns the checksum (of the requested type) of the protocol's contents

  Useful for protocols which have a checksum field 

  \note If a subclass uses protocolFrameCksum() from within fieldData() to
  derive a cksum field, it MUST handle and return the 'FieldBitSize'
  attribute also for that particular field instead of using the default
  AbstractProtocol implementation for 'FieldBitSize' - this is required 
  to prevent infinite recursion
*/
quint32 AbstractProtocol::protocolFrameCksum(int streamIndex,
    CksumType cksumType, CksumFlags cksumFlags) const
{
    static int recursionCount = 0;
    quint32 cksum = 0xFFFFFFFF;

    recursionCount++;
    Q_ASSERT_X(recursionCount < 10, "protocolFrameCksum", "potential infinite recursion - does a protocol checksum field not implement FieldBitSize?");

    switch(cksumType)
    {
        case CksumIp:
        {
            QByteArray fv;
            quint16 *ip;
            quint32 len, sum = 0;
            bool forCksum = cksumFlags.testFlag(IncludeCksumField) ?
                                false : true;

            fv = protocolFrameValue(streamIndex, forCksum);
            ip = (quint16*) fv.constData();
            len = fv.size();

            while(len > 1)
            {
                sum += *ip;
                if(sum & 0x80000000)
                    sum = (sum & 0xFFFF) + (sum >> 16);
                ip++;
                len -= 2;
            }

            if (len)
                sum += (unsigned short) *(unsigned char *)ip;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = qFromBigEndian((quint16) ~sum);
            break;
        }

        case CksumTcpUdp:
        {
            quint16 cks;
            quint32 sum = 0;

            cks = protocolFrameCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;
            cks = protocolFramePayloadCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;
            cks = protocolFrameHeaderCksum(streamIndex, CksumIpPseudo);
            sum += (quint16) ~cks;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = (~sum) & 0xFFFF;
            break;
        }    

        case CksumIcmpIgmp:
        {
            quint16 cks;
            quint32 sum = 0;

            cks = protocolFrameCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;
            cks = protocolFramePayloadCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = (~sum) & 0xFFFF;
            break;
        }

        default:
            qDebug("Unknown cksumType %d", cksumType);
            break;
    }

    recursionCount--;
    return cksum;
}

/*!
  Returns the checksum of the requested type for the protocol's header 

  This is useful for subclasses which needs the header's checksum e.g. TCP/UDP
  require a "Pseudo-IP" checksum. The checksum is limited to the specified
  scope.

  Currently the default implementation supports only type CksumIpPseudo

  \note The default value for cksumScope is different for
  protocolFrameHeaderCksum() and protocolFramePayloadCksum()
*/
quint32 AbstractProtocol::protocolFrameHeaderCksum(int streamIndex, 
    CksumType cksumType, CksumScope cksumScope) const
{
    quint32 sum = 0;
    quint32 cksum;
    AbstractProtocol *p = prev;

    Q_ASSERT(cksumType == CksumIpPseudo);

    // We may have extension headers between us and the IP header - skip 'em
    while (p)
    {
        cksum = p->protocolFrameCksum(streamIndex, cksumType);
        if (cksum <= 0xFFFF) // protocol has a valid pseudo cksum ie its IP
        {
            sum += (quint16) ~cksum;
            // Ip4/6Protocol::protocolFrameCksum(CksumIpPseudo) only
            // counts the src/dst IP (see Note in there)
            // Count the payload length and protocolId here
            sum += protocolFrameSize(streamIndex)
                    + protocolFramePayloadSize(streamIndex);
            sum += protocolId(ProtocolIdIp);
            qDebug("%s: sum = %x, cksum = %x", __FUNCTION__, sum, cksum);
            if (cksumScope == CksumScopeAdjacentProtocol)
                goto out;
        }
        p = p->prev;
    }
    if (parent)
    {
        cksum = parent->protocolFrameHeaderCksum(streamIndex, cksumType, 
                cksumScope);
        sum += (quint16) ~cksum;
    }

out:
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (quint16) ~sum;
}

/*!
  Returns the checksum of the requested type for the protocol's payload

  This is useful for subclasses which needs the payload's checksum e.g. TCP/UDP
  require a IP checksum of the payload (to be combined with other checksums to
  derive the final checksum). The checksum is limited to the specified
  scope.

  Currently the default implementation supports only type CksumIp

  \note The default value for cksumScope is different for
  protocolFrameHeaderCksum() and protocolFramePayloadCksum()
*/
quint32 AbstractProtocol::protocolFramePayloadCksum(int streamIndex,
    CksumType cksumType, CksumScope cksumScope) const
{
    quint32 sum;
    quint16 cksum;
    AbstractProtocol *p = next;

    Q_ASSERT(cksumType == CksumIp);

    if (!p)
        return 0xFFFF;

    cksum = p->protocolFrameCksum(streamIndex, cksumType, IncludeCksumField);
    sum = (quint16) ~cksum;
    if (cksumScope == CksumScopeAdjacentProtocol)
        goto out;

    p = p->next;
    while (p)
    {
        cksum = p->protocolFrameCksum(streamIndex, cksumType, IncludeCksumField);
        // when combining cksums, a non-first protocol starting at odd offset
        // needs a byte swap (see RFC 1071 section(s) 2A, 2B)
        if (p->protocolFrameOffset(streamIndex) & 0x1)
            cksum = swap16(cksum);
        sum += (quint16) ~cksum;
        p = p->next;
    }

    if (parent)
    {
        cksum = parent->protocolFramePayloadCksum(streamIndex, cksumType,
                cksumScope);
        sum += (quint16) ~cksum;
    }

out:
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    cksum = (quint16) ~sum;
    qDebug("%s: cksum = %u", __FUNCTION__, cksum);
    return cksum;
}

/*!
    Returns true, if the protocol fields are incorrect or may cause
    overall packet to be invalid

    Error details are put in the optional INOUT param 'errors', if true.
*/
bool AbstractProtocol::hasErrors(QStringList* /*errors*/) const
{
    return false;
}

// Stein's binary GCD algo - from wikipedia
quint64 AbstractProtocol::gcd(quint64 u, quint64 v)
{
    int shift;

    /* GCD(0,x) := x */
    if (u == 0 || v == 0)
        return u | v;

    /* Let shift := lg K, where K is the greatest power of 2
       dividing both u and v. */
    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

    while ((u & 1) == 0)
        u >>= 1;

    /* From here on, u is always odd. */
    do {
        while ((v & 1) == 0)  /* Loop X */
            v >>= 1;

        /* Now u and v are both odd, so diff(u, v) is even.
           Let u = min(u, v), v = diff(u, v)/2. */
        if (u < v) {
            v -= u;
        } else {
            quint64 diff = u - v;
            u = v;
            v = diff;
        }
        v >>= 1;
    } while (v != 0);

    return u << shift;
}

quint64 AbstractProtocol::lcm(quint64 u, quint64 v)
{
#if 0
    /* LCM(0,x) := x */
    if (u == 0 || v == 0)
        return u | v;
#else
    /* For our use case, neither u nor v can ever be 0, the minimum
       value is 1; we do this correction silently here */
    if (u == 0) u = 1;
    if (v == 0) v = 1;

    if (u == 1 || v == 1)
        return (u * v);
#endif

    return (u * v)/gcd(u, v);
}

/* 
 * XXX: varyCounter() is not a member of AbstractProtocol to avoid
 * moving it into the header file and thereby keeping the header file
 * clean
 */
template <typename T>
bool varyCounter(QString protocolName, QByteArray &buf, int frameIndex, 
                 const OstProto::VariableField &varField)
{
    int x = (frameIndex % varField.count()) * varField.step();

    T oldfv, newfv;

    if ((varField.offset() + sizeof(T)) > uint(buf.size()))
    {
        qWarning("%s varField ofs %d beyond protocol frame %d - skipping", 
                qPrintable(protocolName), varField.offset(), buf.size());
        return false;
    }

    oldfv = *((T*)((uchar*)buf.constData() + varField.offset()));
    if (sizeof(T) > sizeof(quint8))
        oldfv = qFromBigEndian(oldfv);

    switch(varField.mode())
    {
        case OstProto::VariableField::kIncrement:
            newfv = (oldfv & ~varField.mask()) 
                | ((varField.value() + x) & varField.mask());
            break;
        case OstProto::VariableField::kDecrement:
            newfv = (oldfv & ~varField.mask()) 
                | ((varField.value() - x) & varField.mask());
            break;
        case OstProto::VariableField::kRandom:
            newfv = (oldfv & ~varField.mask()) 
                | ((varField.value() + qrand()) & varField.mask());
            break;
        default:
            qWarning("%s Unsupported varField mode %d", 
                    qPrintable(protocolName), varField.mode());
            return false;
    }

    if (sizeof(T) == sizeof(quint8))
        *((uchar*)buf.constData() + varField.offset()) = newfv;
    else
        qToBigEndian(newfv, (uchar*)buf.constData() + varField.offset());

    qDebug("%s varField ofs %d oldfv %x newfv %x", 
            qPrintable(protocolName), varField.offset(), oldfv, newfv);
    return true;
}

void AbstractProtocol::varyProtocolFrameValue(QByteArray &buf, int frameIndex,
        const OstProto::VariableField &varField) const
{

    switch (varField.type()) {
    case OstProto::VariableField::kCounter8:
        varyCounter<quint8>(shortName(), buf, frameIndex, varField);
        break;
    case OstProto::VariableField::kCounter16:
        varyCounter<quint16>(shortName(), buf, frameIndex, varField);
        break;
    case OstProto::VariableField::kCounter32:
        varyCounter<quint32>(shortName(), buf, frameIndex, varField);
        break;
    default:
        break;
    }

    return;
}

