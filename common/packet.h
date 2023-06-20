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

#ifndef _PACKET_H
#define _PACKET_H

#include <QSet>
#include <QtGlobal>

namespace Packet {

class Parser {
public:
    Parser(const uchar *data, int length)
        : pktData_(data), pktLen_(length) {}
    quint8  field8(int offset) {
        if (offset >= pktLen_) {
            ok_ = false;
            return 0;
        }
        ok_ = true;
        return pktData_[offset];
    }
    quint16 field16(int offset) {
        if (offset + 1 >= pktLen_) {
            ok_ = false;
            return 0;
        }
        ok_ = true;
        return pktData_[offset] << 8
                | pktData_[offset+1];
    }
    quint32 field32(int offset) {
        if (offset + 3 >= pktLen_) {
            ok_ = false;
            return 0;
        }
        ok_ = true;
        return pktData_[offset] << 24
                | pktData_[offset+1] << 16
                | pktData_[offset+2] << 8
                | pktData_[offset+3];
    }
    bool ok() {
        return ok_;
    }
private:
    const uchar *pktData_;
    int pktLen_;
    bool ok_{false};
};

quint16 l4ChecksumOffset(const uchar *pktData, int pktLen);

//
// Constants
//
// Ethernet
const quint16 kEthTypeOffset = 12;
const quint16 kEthTypeSize = 2;
const quint16 kIp4EthType = 0x0800;
const quint16 kIp6EthType = 0x86dd;
const quint16 kMplsEthType = 0x8847;
const QSet<quint16> kVlanEthTypes = {0x8100, 0x9100, 0x88a8};
const uint kEthOverhead = 20;

// VLAN
const quint16 kVlanTagSize = 4;

// MPLS
const quint16 kMplsTagSize = 4;

// IPv4
const quint16 kIp4ProtocolOffset = 9;

// IPv6
const quint16 kIp6HeaderSize = 40;
const quint16 kIp6NextHeaderOffset = 6;

// IPv6 Extension Header
const quint16 kIp6ExtNextHeaderOffset = 0;
const quint16 kIp6ExtLengthOffset = 1;

// IPv4/IPv6 Proto/NextHeader values
const quint8 kIpProtoTcp = 6;
const quint8 kIpProtoUdp = 17;

const QSet<quint8> kIp6ExtensionHeaders = {0, 60, 43, 44, 51, 50, 60, 135}; // FIXME: use names

// TCP
const quint16 kTcpChecksumOffset = 16;

// UDP
const quint16 kUdpChecksumOffset = 6;
};

#endif
