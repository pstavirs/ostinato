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

#ifndef _ABSTRACT_PROTOCOL_H
#define _ABSTRACT_PROTOCOL_H

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QLinkedList>
#include <QFlags>
#include <qendian.h>

//#include "../rpc/pbhelper.h"
#include "protocol.pb.h"

#define BASE_BIN (2)
#define BASE_OCT (8)
#define BASE_DEC (10)
#define BASE_HEX (16)

class StreamBase;
class ProtocolListIterator;

class AbstractProtocol
{
    template <int protoNumber, class ProtoA, class ProtoB> 
        friend class ComboProtocol;
    friend class ProtocolListIterator;

private:
    mutable int _metaFieldCount;
    mutable int _frameFieldCount;
    mutable int protoSize;
    mutable QString protoAbbr;

protected:
    StreamBase          *mpStream; //!< Stream that this protocol belongs to
    AbstractProtocol    *parent;   //!< Parent protocol, if any
    AbstractProtocol    *prev;     //!< Protocol preceding this protocol
    AbstractProtocol    *next;     //!< Protocol succeeding this protocol 

    //! Is protocol typically followed by payload or another protocol
    bool _hasPayload;

public:
    //! Properties of a field, can be OR'd
    enum FieldFlag {
        FrameField = 0x1, //!< field appears in frame content
        MetaField  = 0x2, //!< field does not appear in frame, is meta data
        CksumField = 0x4  //!< field is a checksum and appears in frame content
    };
    Q_DECLARE_FLAGS(FieldFlags, FieldFlag);  //!< \private abcd

    //! Various attributes of a field
    enum FieldAttrib {
        FieldName,          //!< name
        FieldValue,         //!< value in host byte order (user editable)
        FieldTextValue,     //!< value as text
        FieldFrameValue,    //!< frame encoded value in network byte order
        FieldBitSize,       //!< size in bits
    };

    //! Supported Protocol Id types 
    enum ProtocolIdType {
        ProtocolIdNone,     //!< Marker representing non-existent protocol id
        ProtocolIdLlc,      //!< LLC (802.2)
        ProtocolIdEth,      //!< Ethernet II
        ProtocolIdIp,       //!< IP
        ProtocolIdTcpUdp,   //!< TCP/UDP Port Number
    };

    //! Supported checksum types 
    enum CksumType {
        CksumIp,        //!< Standard IP Checksum
        CksumIpPseudo,  //!< Standard checksum for Pseudo-IP header
        CksumTcpUdp,    //!< Standard TCP/UDP checksum including pseudo-IP

        CksumMax        //!< Marker for number of cksum types
    };

    //! Supported checksum scopes  
    enum CksumScope {
        CksumScopeAdjacentProtocol,   //!< Cksum only the adjacent protocol
        CksumScopeAllProtocols,       //!< Cksum over all the protocols
    };

    AbstractProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~AbstractProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const = 0;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol) = 0;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual ProtocolIdType protocolIdType() const;
    virtual quint32 protocolId(ProtocolIdType type) const;
    quint32 payloadProtocolId(ProtocolIdType type) const;

    virtual int    fieldCount() const;
    int metaFieldCount() const;
    virtual int frameFieldCount() const;

    virtual FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
        int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib = FieldValue);

    QByteArray protocolFrameValue(int streamIndex = 0,
        bool forCksum = false) const;
    virtual int protocolFrameSize(int streamIndex = 0) const;
    int protocolFrameOffset(int streamIndex = 0) const;
    int protocolFramePayloadSize(int streamIndex = 0) const;

    virtual bool isProtocolFrameValueVariable() const;
    virtual bool isProtocolFrameSizeVariable() const;
    virtual int protocolFrameVariableCount() const;
    bool isProtocolFramePayloadValueVariable() const;
    bool isProtocolFramePayloadSizeVariable() const;
    int protocolFramePayloadVariableCount() const;

    bool protocolHasPayload() const;

    virtual quint32 protocolFrameCksum(int streamIndex = 0,
        CksumType cksumType = CksumIp) const;
    quint32 protocolFrameHeaderCksum(int streamIndex = 0,
        CksumType cksumType = CksumIp,
        CksumScope cksumScope = CksumScopeAdjacentProtocol) const;
    quint32 protocolFramePayloadCksum(int streamIndex = 0,
        CksumType cksumType = CksumIp,
        CksumScope cksumScope = CksumScopeAllProtocols) const;

    static quint64 lcm(quint64 u, quint64 v);
    static quint64 gcd(quint64 u, quint64 v);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractProtocol::FieldFlags);

#endif
