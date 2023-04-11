/*
Copyright (C) 2023 Srivats P.

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

#include "packet.h"

using namespace Packet;

quint16 Packet::l4ChecksumOffset(const uchar *pktData, int pktLen)
{
    Parser parser(pktData, pktLen);
    quint16 offset = kEthTypeOffset;

    // Skip VLANs, if any
    quint16 ethType = parser.field16(offset);
    if (!parser.ok()) return 0;

    // TODO: support 802.3 frames
    if (ethType <= 1500)
        return 0;

    while (kVlanEthTypes.contains(ethType)) {
        offset += kVlanTagSize;
        ethType = parser.field16(offset);
        if (!parser.ok()) return 0;
    }
    offset += kEthTypeSize;

    // XXX: offset now points to Eth payload

    // Skip MPLS tags, if any
    if (ethType == kMplsEthType) {
        while (1) {
            quint32 mplsTag = parser.field32(offset);
            if (!parser.ok()) return 0;
            offset += kMplsTagSize;
            if (mplsTag & 0x100) { // BOS bit
                quint32 nextWord = parser.field32(offset);
                if (!parser.ok()) return 0;
                if (nextWord == 0) { // PW Control Word
                    offset += kMplsTagSize;
                    ethType = 0;
                    break;
                }
                quint8 firstPayloadNibble = nextWord >> 28;
                if (firstPayloadNibble == 0x4)
                    ethType = kIp4EthType;
                else if (firstPayloadNibble == 0x6)
                    ethType = kIp6EthType;
                else
                    ethType = 0;
                break;
            }
        }
    }

    quint8 ipProto = 0;
    if (ethType == kIp4EthType) {
        ipProto = parser.field8(offset + kIp4ProtocolOffset);
        if (!parser.ok()) return 0;

        quint8 ipHdrLen = parser.field8(offset) & 0x0F;
        if (!parser.ok()) return 0;
        offset += 4*ipHdrLen;
    } else if (ethType == kIp6EthType) {
        ipProto = parser.field8(offset + kIp6NextHeaderOffset);
        if (!parser.ok()) return 0;
        offset += kIp6HeaderSize;

        // XXX: offset now points to IPv6 payload

        // Skip IPv6 extension headers, if any
        while (kIp6ExtensionHeaders.contains(ipProto)) {
            ipProto = parser.field8(offset + kIp6ExtNextHeaderOffset);
            if (!parser.ok()) return 0;

            quint16 extHdrLen = parser.field8(offset + kIp6ExtLengthOffset);
            offset += 8 + 8*extHdrLen;
        }
    } else {
        // Non-IP
        // TODO: support MPLS PW with Eth payload
        return 0;
    }

    // XXX: offset now points to IP payload

    if (ipProto == kIpProtoTcp) {
        parser.field16(offset + kTcpChecksumOffset);
        if (!parser.ok()) return 0;

        return offset + kTcpChecksumOffset;
    } else if (ipProto == kIpProtoUdp) {
        parser.field16(offset + kUdpChecksumOffset);
        if (!parser.ok()) return 0;

        return offset + kUdpChecksumOffset;
    }

    // No L4
    return 0;
}
