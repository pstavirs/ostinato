/*
Copyright (C) 2016 Srivats P.

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

#ifndef _SIGN_H
#define _SIGN_H

#include "abstractprotocol.h"
#include "sign.pb.h"

#include <limits.h>

/*
Sign Protocol is expected at the end of the frame (just before the Eth FCS)
   ---+--------+-------+
 . . .| TLV(s) | Magic |
      |  (8+)  |  (32) |
   ---+--------+-------+
Figures in brackets represent field width in bits

TLVs are encoded as
 +-------+-----+------+
 | Value | Len | Type |
 | (...) | (3) | (5)  |
 +-------+-----+------+
 Len does NOT include the one byte of TypeLen
 Size of the value field varies between 0 to 7 bytes

Defined TLVs
 Type = 0, Len = 0 (0x00): End of TLVs
 Type = 1, Len = 3 (0x61): Stream GUID
 Type = 2, Len = 1 (0x22): T-Tag Placeholder (0 value)
 Type = 3, Len = 1 (0x23): T-Tag with actual value
 Type = 4, Len = 1 (0x24): Tx Port Id

Order of TLVs from end of packet towards beginning [Offset, Size]
 [ -4, 4 bytes] Magic
 [ -6, 2 bytes] TTag (Placeholder or actual)
 [-10, 4 bytes] Stream Guid
 [-12, 2 bytes] Tx Port Id
 [-13, 1 byte ] End
*/

class SignProtocol : public AbstractProtocol
{
public:
    enum samplefield
    {
        // Frame Fields
        sign_tlv_end = 0,
        sign_tlv_tx_port,
        sign_tlv_guid,
        sign_tlv_ttag,
        sign_magic,

        // Meta Fields
        // - None

        sign_fieldCount
    };

    SignProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~SignProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value,
            FieldAttrib attrib = FieldValue);

    static quint32 magic();
    static bool packetGuid(const uchar *pkt, int pktLen, uint *guid);
    static bool packetTtagId(const uchar *pkt, int pktLen, uint *ttagId, uint *guid);

    // XXX: Any change in kTypeLenXXX or magic value should also be done in
    // TxThread/Ttag code as well where hardcoded values are used
    static const quint32 kMaxGuid = 0x00ffffff;
    static const quint32 kInvalidGuid = UINT_MAX;
    static const quint8 kTypeLenTtagPlaceholder = 0x22;
    static const quint8 kTypeLenTtag = 0x23;
private:
    static const quint32 kSignMagic = 0x1d10c0da; // coda! (unicode - 0x1d10c)
    static const quint8 kTypeLenEnd = 0x00;
    static const quint8 kTypeLenGuid = 0x61;
    static const quint8 kTypeLenTxPort = 0x24;
    OstProto::Sign data;
};

#endif
