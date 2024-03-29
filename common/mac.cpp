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

#include "mac.h"

#include "framevalueattrib.h"
#include "../common/streambase.h"

#include <QRegExp>

#define uintToMacStr(num)    \
    QString("%1").arg(num, 6*2, BASE_HEX, QChar('0')) \
        .replace(QRegExp("([0-9a-fA-F]{2}\\B)"), "\\1:").toUpper()

MacProtocol::MacProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    forResolve_ = false;
}

MacProtocol::~MacProtocol()
{
}

AbstractProtocol* MacProtocol::createInstance(StreamBase *stream
    , AbstractProtocol *parent)
{
    return new MacProtocol(stream, parent);
}

quint32 MacProtocol::protocolNumber() const
{
    return OstProto::Protocol::kMacFieldNumber;
}

void MacProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::mac)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void MacProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() && 
            protocol.HasExtension(OstProto::mac))
        data.MergeFrom(protocol.GetExtension(OstProto::mac));
}

QString MacProtocol::name() const
{
    return QString("Media Access Protocol");
}

QString MacProtocol::shortName() const
{
    return QString("MAC");
}

int    MacProtocol::fieldCount() const
{
    return mac_fieldCount;
}

AbstractProtocol::FieldFlags MacProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case mac_dstAddr:
        case mac_srcAddr:
            break;

        case mac_dstMacMode:
        case mac_dstMacCount:
        case mac_dstMacStep:
        case mac_srcMacMode:
        case mac_srcMacCount:
        case mac_srcMacStep:
            flags &= ~FrameField;
            flags |= MetaField;
            break;
    }

    return flags;
}

QVariant MacProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case mac_dstAddr:
        {
            int u;
            quint64 dstMac = 0;

            switch (data.dst_mac_mode())
            {
                case OstProto::Mac::e_mm_fixed:
                    dstMac = data.dst_mac();
                    break;
                case OstProto::Mac::e_mm_inc:
                    u = (streamIndex % data.dst_mac_count()) * 
                        data.dst_mac_step(); 
                    dstMac = data.dst_mac() + u;
                    break;
                case OstProto::Mac::e_mm_dec:
                    u = (streamIndex % data.dst_mac_count()) * 
                        data.dst_mac_step(); 
                    dstMac = data.dst_mac() - u;
                    break;
                case OstProto::Mac::e_mm_resolve:
                    if (forResolve_)
                        dstMac = 0;
                    else {
                        forResolve_ = true;
                        dstMac = mpStream->neighborMacAddress(streamIndex);
                        forResolve_ = false;
                    }
                    break;
                default:
                    qWarning("Unhandled dstMac_mode %d", data.dst_mac_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Destination");
                case FieldValue:
                    return dstMac;
                case FieldTextValue:
                    return uintToMacStr(dstMac);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian(dstMac, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case mac_srcAddr:
        {
            int u;
            quint64 srcMac = 0;

            switch (data.src_mac_mode())
            {
                case OstProto::Mac::e_mm_fixed:
                    srcMac = data.src_mac();
                    break;
                case OstProto::Mac::e_mm_inc:
                    u = (streamIndex % data.src_mac_count()) * 
                        data.src_mac_step(); 
                    srcMac = data.src_mac() + u;
                    break;
                case OstProto::Mac::e_mm_dec:
                    u = (streamIndex % data.src_mac_count()) * 
                        data.src_mac_step(); 
                    srcMac = data.src_mac() - u;
                    break;
                case OstProto::Mac::e_mm_resolve:
                    if (forResolve_)
                        srcMac = 0;
                    else {
                        forResolve_ = true;
                        srcMac = mpStream->deviceMacAddress(streamIndex);
                        forResolve_ = false;
                    }
                    break;
                default:
                    qWarning("Unhandled srcMac_mode %d", data.src_mac_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Source");
                case FieldValue:
                    return srcMac;
                case FieldTextValue:
                    return uintToMacStr(srcMac);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian(srcMac, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }

        // Meta fields
        case mac_dstMacMode:
            switch(attrib)
            {
                case FieldValue: return data.dst_mac_mode();
                default: break;
            }
            break;
        case mac_dstMacCount:
            switch(attrib)
            {
                case FieldValue: return data.dst_mac_count();
                default: break;
            }
            break;
        case mac_dstMacStep:
            switch(attrib)
            {
                case FieldValue: return data.dst_mac_step();
                default: break;
            }
            break;
        case mac_srcMacMode:
            switch(attrib)
            {
                case FieldValue: return data.src_mac_mode();
                default: break;
            }
            break;
        case mac_srcMacCount:
            switch(attrib)
            {
                case FieldValue: return data.src_mac_count();
                default: break;
            }
            break;
        case mac_srcMacStep:
            switch(attrib)
            {
                case FieldValue: return data.src_mac_step();
                default: break;
            }
            break;
        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool MacProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case mac_dstAddr:
        {
            quint64 mac = value.toULongLong(&isOk);
            if (isOk)
                data.set_dst_mac(mac);
            break;
        }
        case mac_srcAddr:
        {
            quint64 mac = value.toULongLong(&isOk);
            if (isOk)
                data.set_src_mac(mac);
            break;
        }

        // Meta-Fields
        case mac_dstMacMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.MacAddrMode_IsValid(mode))
                data.set_dst_mac_mode((OstProto::Mac::MacAddrMode) mode);
            else
                isOk = false;
            break;
        }
        case mac_dstMacCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_mac_count(count);
            break;
        }
        case mac_dstMacStep:
        {
            uint step = value.toUInt(&isOk);
            if (isOk)
                data.set_dst_mac_step(step);
            break;
        }
        case mac_srcMacMode:
        {
            uint mode = value.toUInt(&isOk);
            if (isOk && data.MacAddrMode_IsValid(mode))
                data.set_src_mac_mode((OstProto::Mac::MacAddrMode) mode);
            else
                isOk = false;
            break;
        }
        case mac_srcMacCount:
        {
            uint count = value.toUInt(&isOk);
            if (isOk)
                data.set_src_mac_count(count);
            break;
        }
        case mac_srcMacStep:
        {
            uint step = value.toUInt(&isOk);
            if (isOk)
                data.set_src_mac_step(step);
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

int MacProtocol::protocolFrameVariableCount() const
{
    int count = AbstractProtocol::protocolFrameVariableCount();

    switch (data.dst_mac_mode()) {
        case OstProto::Mac::e_mm_inc:
        case OstProto::Mac::e_mm_dec:
            count = AbstractProtocol::lcm(count, data.dst_mac_count());
            break;
        default:
            break;
    }

    switch (data.src_mac_mode()) {
        case OstProto::Mac::e_mm_inc:
        case OstProto::Mac::e_mm_dec:
            count = AbstractProtocol::lcm(count, data.src_mac_count());
            break;
        default:
            break;
    }

    return count;
}

QByteArray MacProtocol::protocolFrameValue(int streamIndex, bool /*forCksum*/,
        FrameValueAttrib *attrib) const
{
    QByteArray ba;
    ba.resize(12);
    quint64 dstMac = fieldData(mac_dstAddr, FieldValue, streamIndex)
                        .toULongLong();
    quint64 srcMac = fieldData(mac_srcAddr, FieldValue, streamIndex)
                        .toULongLong();
    char *p = ba.data();
    *(quint32*)(p     ) =  qToBigEndian(quint32(dstMac >> 16));
    *(quint16*)(p +  4) =  qToBigEndian(quint16(dstMac & 0xffff));
    *(quint32*)(p +  6) =  qToBigEndian(quint32(srcMac >> 16));
    *(quint16*)(p + 10) =  qToBigEndian(quint16(srcMac & 0xffff));

    if (attrib) {
        if (!dstMac && data.dst_mac_mode() == OstProto::Mac::e_mm_resolve)
            attrib->errorFlags |= FrameValueAttrib::UnresolvedDstMacError;
        if (!srcMac && data.src_mac_mode() == OstProto::Mac::e_mm_resolve)
            attrib->errorFlags |= FrameValueAttrib::UnresolvedSrcMacError;
    }

    return ba;
}

bool MacProtocol::hasErrors(QStringList *errors) const
{
    bool result = false;

    if ((data.dst_mac() == 0ULL)
            && (data.dst_mac_mode() != OstProto::Mac::e_mm_resolve)) {
        if (errors)
            *errors << QObject::tr("Frames with Destination Mac "
                                   "00:00:00:00:00:00 are likely "
                                   "to be dropped");
        result = true;
    }

    if ((data.src_mac() == 0ULL)
            && (data.src_mac_mode() != OstProto::Mac::e_mm_resolve)) {
        if (errors)
            *errors << QObject::tr("Frames with Source Mac "
                                   "00:00:00:00:00:00 are likely "
                                   "to be dropped");
        result = true;
    }

    return result;
}
