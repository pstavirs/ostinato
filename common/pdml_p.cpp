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

#include "pdml_p.h"

#include "protocolmanager.h"

#include "arp.pb.h"
#include "eth2.pb.h"
#include "dot3.pb.h"
#include "gmp.pb.h"
#include "hexdump.pb.h"
#include "llc.pb.h"
#include "mac.pb.h"
#include "icmp.pb.h"
#include "igmp.pb.h"
#include "ip4.pb.h"
#include "ip6.pb.h"
#include "mld.pb.h"
#include "sample.pb.h"
#include "snap.pb.h"
#include "svlan.pb.h"
#include "tcp.pb.h"
#include "textproto.pb.h"
#include "udp.pb.h"
#include "vlan.pb.h"

//#include <QMessageBox>
#include <QRegExp>

#include <string>

extern ProtocolManager *OstProtocolManager;

const int kBaseHex = 16;

//static PdmlReader *gPdmlReader = NULL;

// ---------------------------------------------------------- //
// PdmlUnknownProtocol                                        //
// ---------------------------------------------------------- //

PdmlUnknownProtocol::PdmlUnknownProtocol()
{
    pdmlProtoName_ = "";
    ostProtoId_ = OstProto::Protocol::kHexDumpFieldNumber;

    endPos_ = expPos_ = -1;
}

PdmlProtocol* PdmlUnknownProtocol::createInstance()
{
    return new PdmlUnknownProtocol();
}

void PdmlUnknownProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
{
    bool isOk;
    int size;
    int pos = attributes.value("pos").toString().toUInt(&isOk);
    if (!isOk)
    {
        if (expectedPos >= 0)
            expPos_ = pos = expectedPos;
        else
            goto _skip_pos_size_proc;
    }

    size = attributes.value("size").toString().toUInt(&isOk);
    if (!isOk)
        goto _skip_pos_size_proc;

    // If pos+size goes beyond the frame length, this is a "reassembled"
    // protocol and should be skipped
    if ((pos + size) > int(stream->core().frame_len()))
        goto _skip_pos_size_proc;

    expPos_ = pos;
    endPos_ = expPos_ + size;

_skip_pos_size_proc:
    OstProto::HexDump *hexDump = stream->mutable_protocol(
            stream->protocol_size()-1)->MutableExtension(OstProto::hexDump);
    hexDump->set_pad_until_end(false);
}

void PdmlUnknownProtocol::prematureEndHandler(int pos, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream* /*stream*/)
{
    endPos_ = pos;
}

void PdmlUnknownProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::HexDump *hexDump = pbProto->MutableExtension(OstProto::hexDump);

    // Skipped field(s) at end? Pad with zero!
    if (endPos_ > expPos_)
    {
        QByteArray hexVal(endPos_ - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    qDebug("  hexdump: expPos_ = %d, endPos_ = %d\n", expPos_, endPos_); 

    // If empty for some reason, remove the protocol
    if (hexDump->content().size() == 0)
        stream->mutable_protocol()->RemoveLast();

    endPos_ = expPos_ = -1;
}

void PdmlUnknownProtocol::unknownFieldHandler(QString name, int pos, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    OstProto::HexDump *hexDump = pbProto->MutableExtension(OstProto::hexDump);

    qDebug("  hexdump: %s, pos = %d, expPos_ = %d, endPos_ = %d\n", 
            name.toAscii().constData(), 
            pos, expPos_, endPos_); 

    // Skipped field? Pad with zero!
    if ((pos > expPos_) && (expPos_ < endPos_))
    {
        QByteArray hexVal(pos - expPos_, char(0));

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }

    if (pos == expPos_)
    {
        QByteArray hexVal = attributes.value("unmaskedvalue").isEmpty() ?
                QByteArray::fromHex(attributes.value("value").toString().toUtf8()) :
                QByteArray::fromHex(attributes.value("unmaskedvalue").toString().toUtf8());

        hexDump->mutable_content()->append(hexVal.constData(), hexVal.size());
        expPos_ += hexVal.size();
    }
}


// ---------------------------------------------------------- //
// PdmlGenInfoProtocol                                        //
// ---------------------------------------------------------- //

PdmlGenInfoProtocol::PdmlGenInfoProtocol()
{
    pdmlProtoName_ = "geninfo";
}

PdmlProtocol* PdmlGenInfoProtocol::createInstance()
{
    return new PdmlGenInfoProtocol();
}

#if 0 // done in frame proto
void PdmlGenInfoProtocol::unknownFieldHandler(QString name, int pos, 
        int size, const QXmlStreamAttributes &attributes, OstProto::Stream *stream)
{
    if (name == "len")
        stream->mutable_core()->set_frame_len(size+4); // TODO:check FCS
}
#endif

// ---------------------------------------------------------- //
// PdmlFrameProtocol                                          //
// ---------------------------------------------------------- //

PdmlFrameProtocol::PdmlFrameProtocol()
{
    pdmlProtoName_ = "frame";
}

PdmlProtocol* PdmlFrameProtocol::createInstance()
{
    return new PdmlFrameProtocol();
}

void PdmlFrameProtocol::unknownFieldHandler(QString name, int /*pos*/,
        int /*size*/, const QXmlStreamAttributes &attributes, 
        OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
{
    if (name == "frame.len")
    {
        int len = -1;

        if (!attributes.value("show").isEmpty())
            len = attributes.value("show").toString().toInt();
        stream->mutable_core()->set_frame_len(len+4); // TODO:check FCS
    }
    else if (name == "frame.time_delta")
    {
        if (!attributes.value("show").isEmpty())
        {
            QString delta = attributes.value("show").toString();
            int decimal = delta.indexOf('.');
            
            if (decimal >= 0)
            {
                const uint kNsecsInSec = 1000000000;
                uint sec = delta.left(decimal).toUInt();
                uint nsec = delta.mid(decimal+1).toUInt();
                uint ipg = sec*kNsecsInSec + nsec;
                
                if (ipg)
                {
                    stream->mutable_control()->set_packets_per_sec(
                            kNsecsInSec/ipg);
                }

                qDebug("sec.nsec = %u.%u, ipg = %u", sec, nsec, ipg);
            }
        }
    }
}


// ---------------------------------------------------------- //
// PdmlSvlanProtocol                                          //
// ---------------------------------------------------------- //

PdmlSvlanProtocol::PdmlSvlanProtocol()
{
    pdmlProtoName_ = "ieee8021ad";
    ostProtoId_ = OstProto::Protocol::kSvlanFieldNumber;
}

PdmlProtocol* PdmlSvlanProtocol::createInstance()
{
    return new PdmlSvlanProtocol();
}

void PdmlSvlanProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);

    svlan->set_tpid(0x88a8);
    svlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes svlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the svlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kSvlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlSvlanProtocol::unknownFieldHandler(QString name, int /*pos*/,
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if ((name == "ieee8021ad.id") || (name == "ieee8021ad.svid"))
    {
        bool isOk;
        OstProto::Vlan *svlan = pbProto->MutableExtension(OstProto::svlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ad.cvid")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kSvlanFieldNumber);

        OstProto::Vlan *svlan = proto->MutableExtension(OstProto::svlan);

        svlan->set_tpid(0x88a8);
        svlan->set_is_override_tpid(true);

        bool isOk;
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        svlan->set_vlan_tag(tag);
    }
    else if (name == "ieee8021ah.etype") // yes 'ah' not 'ad' - not a typo!
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        bool isOk;
        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(attributes.value("value")
                .toString().toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
    }
}


// ---------------------------------------------------------- //
// PdmlVlanProtocol                                            //
// ---------------------------------------------------------- //

PdmlVlanProtocol::PdmlVlanProtocol()
{
    pdmlProtoName_ = "vlan";
    ostProtoId_ = OstProto::Protocol::kVlanFieldNumber;
}

PdmlProtocol* PdmlVlanProtocol::createInstance()
{
    return new PdmlVlanProtocol();
}

void PdmlVlanProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);

    vlan->set_tpid(0x8100);
    vlan->set_is_override_tpid(true);

    // If a eth2 protocol precedes vlan, we remove the eth2 protocol
    // 'coz the eth2.etherType is actually the vlan.tpid 
    //
    // We assume that the current protocol is the last in the stream
    int index = stream->protocol_size() - 1;
    if ((index > 1) 
            && (stream->protocol(index).protocol_id().id() 
                                    == OstProto::Protocol::kVlanFieldNumber)
            && (stream->protocol(index - 1).protocol_id().id() 
                                    == OstProto::Protocol::kEth2FieldNumber))
    {
        stream->mutable_protocol()->SwapElements(index, index - 1);
        Q_ASSERT(stream->protocol(index).protocol_id().id()
                        == OstProto::Protocol::kEth2FieldNumber);
        stream->mutable_protocol()->RemoveLast();
    }
}

void PdmlVlanProtocol::unknownFieldHandler(QString name, int /*pos*/,
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "vlan.id")
    {
        bool isOk;
        OstProto::Vlan *vlan = pbProto->MutableExtension(OstProto::vlan);
        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        vlan->set_vlan_tag(tag);
    }
    else if (name == "vlan.etype")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        bool isOk;
        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(attributes.value("value")
                .toString().toUInt(&isOk, kBaseHex));
        eth2->set_is_override_type(true);
    }
}


// ---------------------------------------------------------- //
// PdmlEthProtocol                                            //
// ---------------------------------------------------------- //

PdmlEthProtocol::PdmlEthProtocol()
{
    pdmlProtoName_ = "eth";
    ostProtoId_ = OstProto::Protocol::kMacFieldNumber;

    fieldMap_.insert("eth.dst", OstProto::Mac::kDstMacFieldNumber);
    fieldMap_.insert("eth.src", OstProto::Mac::kSrcMacFieldNumber);
}

PdmlProtocol* PdmlEthProtocol::createInstance()
{
    return new PdmlEthProtocol();
}

void PdmlEthProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
{
    if (name == "eth.vlan.tpid")
    {
        bool isOk;

        uint tpid = attributes.value("value").toString()
            .toUInt(&isOk, kBaseHex);

        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kVlanFieldNumber);

        OstProto::Vlan *vlan = proto->MutableExtension(OstProto::vlan);

        vlan->set_tpid(tpid);
        vlan->set_is_override_tpid(true);
    }
    else if (name == "eth.vlan.id")
    {
        bool isOk;

        uint tag = attributes.value("unmaskedvalue").isEmpty() ?
            attributes.value("value").toString().toUInt(&isOk, kBaseHex) :
            attributes.value("unmaskedvalue").toString().toUInt(&isOk,kBaseHex);

        OstProto::Vlan *vlan = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::vlan);

        vlan->set_vlan_tag(tag);
    }
    else if (name == "eth.type")
    {
        bool isOk;

        uint type = attributes.value("value").toString()
            .toUInt(&isOk, kBaseHex);

        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kEth2FieldNumber);

        OstProto::Eth2 *eth2 = proto->MutableExtension(OstProto::eth2);

        eth2->set_type(type);
        eth2->set_is_override_type(true);
    }
    else if (name == "eth.len")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kDot3FieldNumber);

        OstProto::Dot3 *dot3 = proto->MutableExtension(OstProto::dot3);

        bool isOk;
        dot3->set_length(attributes.value("value").toString().toUInt(&isOk, kBaseHex));
        dot3->set_is_override_length(true);
    }
    else if (name == "eth.trailer")
    {
        QByteArray trailer = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());

        stream->mutable_core()->mutable_name()->append(trailer.constData(),
                trailer.size());
    }
    else if ((name == "eth.fcs") || 
            attributes.value("show").toString().startsWith("Frame check sequence"))
    {
        QByteArray trailer = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());

        stream->mutable_core()->mutable_name()->append(trailer.constData(),
                trailer.size());
    }
}


// ---------------------------------------------------------- //
// PdmlLlcProtocol                                            //
// ---------------------------------------------------------- //

PdmlLlcProtocol::PdmlLlcProtocol()
{
    pdmlProtoName_ = "llc";
    ostProtoId_ = OstProto::Protocol::kLlcFieldNumber;

    fieldMap_.insert("llc.dsap", OstProto::Llc::kDsapFieldNumber);
    fieldMap_.insert("llc.ssap", OstProto::Llc::kSsapFieldNumber);
    fieldMap_.insert("llc.control", OstProto::Llc::kCtlFieldNumber);
}

PdmlProtocol* PdmlLlcProtocol::createInstance()
{
    return new PdmlLlcProtocol();
}

void PdmlLlcProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol* /*pbProto*/, OstProto::Stream *stream)
{
    if (name == "llc.oui")
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kSnapFieldNumber);

        OstProto::Snap *snap = proto->MutableExtension(OstProto::snap);

        bool isOk;
        snap->set_oui(attributes.value("value").toString()
                .toUInt(&isOk, kBaseHex));
        snap->set_is_override_oui(true);
    }
    else if ((name == "llc.type") || (name.contains(QRegExp("llc\\..*pid"))))
    {
        OstProto::Snap *snap = stream->mutable_protocol(
                stream->protocol_size()-1)->MutableExtension(OstProto::snap);

        bool isOk;
        snap->set_type(attributes.value("value").toString()
                .toUInt(&isOk, kBaseHex));
        snap->set_is_override_type(true);
    }
}

void PdmlLlcProtocol::postProtocolHandler(OstProto::Protocol *pbProto, 
        OstProto::Stream* /*stream*/)
{
    OstProto::Llc *llc = pbProto->MutableExtension(OstProto::llc);

    llc->set_is_override_dsap(true);
    llc->set_is_override_ssap(true);
    llc->set_is_override_ctl(true);
}


// ---------------------------------------------------------- //
// PdmlArpProtocol                                            //
// ---------------------------------------------------------- //

PdmlArpProtocol::PdmlArpProtocol()
{
    pdmlProtoName_ = "arp";
    ostProtoId_ = OstProto::Protocol::kArpFieldNumber;

    fieldMap_.insert("arp.opcode", OstProto::Arp::kOpCodeFieldNumber);
    fieldMap_.insert("arp.src.hw_mac", OstProto::Arp::kSenderHwAddrFieldNumber);
    fieldMap_.insert("arp.src.proto_ipv4", 
            OstProto::Arp::kSenderProtoAddrFieldNumber);
    fieldMap_.insert("arp.dst.hw_mac", OstProto::Arp::kTargetHwAddrFieldNumber);
    fieldMap_.insert("arp.dst.proto_ipv4", 
            OstProto::Arp::kTargetProtoAddrFieldNumber);
}

PdmlProtocol* PdmlArpProtocol::createInstance()
{
    return new PdmlArpProtocol();
}


// ---------------------------------------------------------- //
// PdmlIp4Protocol                                            //
// ---------------------------------------------------------- //

PdmlIp4Protocol::PdmlIp4Protocol()
{
    pdmlProtoName_ = "ip";
    ostProtoId_ = OstProto::Protocol::kIp4FieldNumber;

    fieldMap_.insert("ip.version", OstProto::Ip4::kVerHdrlenFieldNumber);
    fieldMap_.insert("ip.dsfield", OstProto::Ip4::kTosFieldNumber);
    fieldMap_.insert("ip.len", OstProto::Ip4::kTotlenFieldNumber);
    fieldMap_.insert("ip.id", OstProto::Ip4::kIdFieldNumber);
    //fieldMap_.insert("ip.flags", OstProto::Ip4::kFlagsFieldNumber);
    fieldMap_.insert("ip.frag_offset", OstProto::Ip4::kFragOfsFieldNumber);
    fieldMap_.insert("ip.ttl", OstProto::Ip4::kTtlFieldNumber);
    fieldMap_.insert("ip.proto", OstProto::Ip4::kProtoFieldNumber);
    fieldMap_.insert("ip.checksum", OstProto::Ip4::kCksumFieldNumber);
    fieldMap_.insert("ip.src", OstProto::Ip4::kSrcIpFieldNumber);
    fieldMap_.insert("ip.dst", OstProto::Ip4::kDstIpFieldNumber);
}

PdmlProtocol* PdmlIp4Protocol::createInstance()
{
    return new PdmlIp4Protocol();
}

void PdmlIp4Protocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;

    if ((name == "ip.options") ||
            attributes.value("show").toString().startsWith("Options"))
    {
        options_ = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());
    }
    else if (name == "ip.flags")
    {
        OstProto::Ip4 *ip4 = pbProto->MutableExtension(OstProto::ip4);

        ip4->set_flags(attributes.value("value").toString().toUInt(&isOk, kBaseHex) >> 5);
    }
}

void PdmlIp4Protocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Ip4 *ip4 = pbProto->MutableExtension(OstProto::ip4);

    ip4->set_is_override_ver(true); // FIXME
    ip4->set_is_override_hdrlen(true); // FIXME
    ip4->set_is_override_totlen(true); // FIXME
    ip4->set_is_override_proto(true); // FIXME
    ip4->set_is_override_cksum(true); // FIXME

    if (options_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(options_.constData(), 
                options_.size());
        hexDump->set_pad_until_end(false);
        options_.resize(0);
    }
}

// ---------------------------------------------------------- //
// PdmlIp6Protocol                                            //
// ---------------------------------------------------------- //

PdmlIp6Protocol::PdmlIp6Protocol()
{
    pdmlProtoName_ = "ipv6";
    ostProtoId_ = OstProto::Protocol::kIp6FieldNumber;

    fieldMap_.insert("ipv6.version", OstProto::Ip6::kVersionFieldNumber);
    fieldMap_.insert("ipv6.class", OstProto::Ip6::kTrafficClassFieldNumber);
    fieldMap_.insert("ipv6.flow", OstProto::Ip6::kFlowLabelFieldNumber);
    fieldMap_.insert("ipv6.plen", OstProto::Ip6::kPayloadLengthFieldNumber);
    fieldMap_.insert("ipv6.nxt", OstProto::Ip6::kNextHeaderFieldNumber);
    fieldMap_.insert("ipv6.hlim", OstProto::Ip6::kHopLimitFieldNumber);

    // ipv6.src and ipv6.dst handled as unknown fields
}

PdmlProtocol* PdmlIp6Protocol::createInstance()
{
    return new PdmlIp6Protocol();
}

void PdmlIp6Protocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;

    if (name == "ipv6.src")
    {
        OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_src_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_src_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "ipv6.dst")
    {
        OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);
        QString addrHexStr = attributes.value("value").toString();

        ip6->set_dst_addr_hi(addrHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip6->set_dst_addr_lo(addrHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
}

void PdmlIp6Protocol::postProtocolHandler(OstProto::Protocol *pbProto, 
        OstProto::Stream* /*stream*/)
{
    OstProto::Ip6 *ip6 = pbProto->MutableExtension(OstProto::ip6);

    ip6->set_is_override_version(true); // FIXME
    ip6->set_is_override_payload_length(true); // FIXME
    ip6->set_is_override_next_header(true); // FIXME
}


// ---------------------------------------------------------- //
// PdmlIcmpProtocol                                            //
// ---------------------------------------------------------- //

PdmlIcmpProtocol::PdmlIcmpProtocol()
{
    pdmlProtoName_ = "icmp";
    ostProtoId_ = OstProto::Protocol::kIcmpFieldNumber;

    fieldMap_.insert("icmp.type", OstProto::Icmp::kTypeFieldNumber);
    fieldMap_.insert("icmp.code", OstProto::Icmp::kCodeFieldNumber);
    fieldMap_.insert("icmp.checksum", OstProto::Icmp::kChecksumFieldNumber);
    fieldMap_.insert("icmp.ident", OstProto::Icmp::kIdentifierFieldNumber);
    fieldMap_.insert("icmp.seq", OstProto::Icmp::kSequenceFieldNumber);

    fieldMap_.insert("icmpv6.type", OstProto::Icmp::kTypeFieldNumber);
    fieldMap_.insert("icmpv6.code", OstProto::Icmp::kCodeFieldNumber);
    fieldMap_.insert("icmpv6.checksum", OstProto::Icmp::kChecksumFieldNumber);
    fieldMap_.insert("icmpv6.echo.identifier", 
            OstProto::Icmp::kIdentifierFieldNumber);
    fieldMap_.insert("icmpv6.echo.sequence_number", 
            OstProto::Icmp::kSequenceFieldNumber);
}

PdmlProtocol* PdmlIcmpProtocol::createInstance()
{
    return new PdmlIcmpProtocol();
}

void PdmlIcmpProtocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if (name == "icmp")
        icmp->set_icmp_version(OstProto::Icmp::kIcmp4);
    else if (name == "icmpv6")
        icmp->set_icmp_version(OstProto::Icmp::kIcmp6);

    icmp->set_is_override_checksum(true);

    icmp->set_type(kIcmpInvalidType);
}

void PdmlIcmpProtocol::unknownFieldHandler(QString /*name*/, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if ((icmp->icmp_version() == OstProto::Icmp::kIcmp6)
            && (icmp->type() >= kIcmp6EchoRequest) 
            && (icmp->type() <= kIcmp6EchoReply))
    {
        QString addrHexStr = attributes.value("value").toString();

        // Wireshark 1.4.x does not have these as filterable fields
        if (attributes.value("show").toString().startsWith("ID"))
            icmp->set_identifier(addrHexStr.toUInt(&isOk, kBaseHex));
        else if (attributes.value("show").toString().startsWith("Sequence"))
            icmp->set_sequence(addrHexStr.toUInt(&isOk, kBaseHex));
    }
}

void PdmlIcmpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Icmp *icmp = pbProto->MutableExtension(OstProto::icmp);

    if (icmp->type() == kIcmpInvalidType)
        stream->mutable_protocol()->RemoveLast();
}

// ---------------------------------------------------------- //
// PdmlIcmp6Protocol                                          //
// ---------------------------------------------------------- //

PdmlIcmp6Protocol::PdmlIcmp6Protocol()
{
    pdmlProtoName_ = "icmpv6";
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;

    proto_ = NULL;
}

PdmlProtocol* PdmlIcmp6Protocol::createInstance()
{
    return new PdmlIcmp6Protocol();
}

void PdmlIcmp6Protocol::preProtocolHandler(QString name, 
        const QXmlStreamAttributes &attributes, 
        int expectedPos, OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    proto_ = NULL;
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;
    icmp_.preProtocolHandler(name, attributes, expectedPos, pbProto, stream);
    mld_.preProtocolHandler(name, attributes, expectedPos, pbProto, stream);
}

void PdmlIcmp6Protocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    if (proto_)
        proto_->postProtocolHandler(pbProto, stream);
    else
        stream->mutable_protocol()->RemoveLast();

    proto_ = NULL;
    ostProtoId_ = OstProto::Protocol::kSampleFieldNumber;
}

void PdmlIcmp6Protocol::unknownFieldHandler(QString name, 
        int pos, int size, const QXmlStreamAttributes &attributes, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (proto_)
    {
        proto_->unknownFieldHandler(name, pos, size, attributes, pbProto, 
            stream);
    }
    else if (name == "icmpv6.type")
    {
        bool isOk;
        uint type =  attributes.value("value").toString().toUInt(
                &isOk, kBaseHex);

        if (((type >= 130) && (type <= 132)) || (type == 143))
        {
            // MLD
            proto_ = &mld_;
            fieldMap_ = mld_.fieldMap_;
            ostProtoId_ = OstProto::Protocol::kMldFieldNumber;
        }
        else
        {
            // ICMP
            proto_ = &icmp_;
            fieldMap_ = icmp_.fieldMap_;
            ostProtoId_ = OstProto::Protocol::kIcmpFieldNumber;
        }

        pbProto->mutable_protocol_id()->set_id(ostProtoId_);
        pbProto->MutableExtension(OstProto::sample)->Clear();

        fieldHandler(name, attributes, pbProto, stream);
    }
    else
    {
        qDebug("unexpected field %s", name.toAscii().constData());
    }
}


// ---------------------------------------------------------- //
// PdmlIgmpProtocol                                            //
// ---------------------------------------------------------- //

PdmlIgmpProtocol::PdmlIgmpProtocol()
{
    pdmlProtoName_ = "igmp";
    ostProtoId_ = OstProto::Protocol::kIgmpFieldNumber;

    fieldMap_.insert("igmp.max_resp", 
            OstProto::Gmp::kMaxResponseTimeFieldNumber); // FIXME
    fieldMap_.insert("igmp.checksum", OstProto::Gmp::kChecksumFieldNumber);

    fieldMap_.insert("igmp.s", OstProto::Gmp::kSFlagFieldNumber);
    fieldMap_.insert("igmp.qrv", OstProto::Gmp::kQrvFieldNumber);
    fieldMap_.insert("igmp.qqic", OstProto::Gmp::kQqiFieldNumber); // FIXME

    fieldMap_.insert("igmp.num_grp_recs", 
            OstProto::Gmp::kGroupRecordCountFieldNumber);
}

PdmlProtocol* PdmlIgmpProtocol::createInstance()
{
    return new PdmlIgmpProtocol();
}

void PdmlIgmpProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes& /*attributes*/, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    OstProto::Gmp *igmp = pbProto->MutableExtension(OstProto::igmp);

    igmp->set_is_override_rsvd_code(true);
    igmp->set_is_override_checksum(true);
    igmp->set_is_override_source_count(true);
    igmp->set_is_override_group_record_count(true);

    version_ = 0;
}

void PdmlIgmpProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Gmp *igmp = pbProto->MutableExtension(OstProto::igmp);
    QString valueHexStr = attributes.value("value").toString();

    if (name == "igmp.version")
    {
        version_ = attributes.value("show").toString().toUInt(&isOk);
    }
    else if (name == "igmp.type")
    {
        uint type =  valueHexStr.toUInt(&isOk, kBaseHex);
        if (type == kIgmpQuery)
        {
            switch(version_)
            {
            case 1: type = kIgmpV1Query; break;
            case 2: type = kIgmpV2Query; break;
            case 3: type = kIgmpV3Query; break;
            }
        }
        igmp->set_type(type);
    }
    else if (name == "igmp.record_type")
    {
        OstProto::Gmp::GroupRecord *rec = igmp->add_group_records();
        rec->set_type(OstProto::Gmp::GroupRecord::RecordType(
                    valueHexStr.toUInt(&isOk, kBaseHex)));
        rec->set_is_override_source_count(true);
        rec->set_is_override_aux_data_length(true);
    }
    else if (name == "igmp.aux_data_len")
    {
        igmp->mutable_group_records(igmp->group_records_size() - 1)->
            set_aux_data_length(valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "igmp.num_src")
    {
        if (igmp->group_record_count())
            igmp->mutable_group_records(igmp->group_records_size() - 1)->
                set_source_count(valueHexStr.toUInt(&isOk, kBaseHex));
        else
            igmp->set_source_count(valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "igmp.maddr")
    {
        if (igmp->group_record_count())
            igmp->mutable_group_records(igmp->group_records_size() - 1)->
                mutable_group_address()->set_v4(
                        valueHexStr.toUInt(&isOk, kBaseHex));
        else
            igmp->mutable_group_address()->set_v4(
                    valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "igmp.saddr")
    {
        if (igmp->group_record_count())
            igmp->mutable_group_records(igmp->group_records_size() - 1)->
                add_sources()->set_v4(valueHexStr.toUInt(&isOk, kBaseHex));
        else
            igmp->add_sources()->set_v4(valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "igmp.aux_data")
    {
        QByteArray ba = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());
        igmp->mutable_group_records(igmp->group_records_size() - 1)->
            set_aux_data(ba.constData(), ba.size());
    }
}

void PdmlIgmpProtocol::postProtocolHandler(OstProto::Protocol* /*pbProto*/,
        OstProto::Stream *stream)
{
    // version is 0 for IGMP like protocols such as RGMP which we don't
    // support currently
    if (version_ == 0)
        stream->mutable_protocol()->RemoveLast();
}


// ---------------------------------------------------------- //
// PdmlMldProtocol                                            //
// ---------------------------------------------------------- //

PdmlMldProtocol::PdmlMldProtocol()
{
    pdmlProtoName_ = "mld";
    ostProtoId_ = OstProto::Protocol::kMldFieldNumber;

    fieldMap_.insert("icmpv6.code", OstProto::Gmp::kRsvdCodeFieldNumber);
    fieldMap_.insert("icmpv6.checksum", OstProto::Gmp::kChecksumFieldNumber);
    fieldMap_.insert("icmpv6.mld.maximum_response_delay", 
            OstProto::Gmp::kMaxResponseTimeFieldNumber); // FIXME

    fieldMap_.insert("icmpv6.mld.flag.s", OstProto::Gmp::kSFlagFieldNumber);
    fieldMap_.insert("icmpv6.mld.flag.qrv", OstProto::Gmp::kQrvFieldNumber);
    fieldMap_.insert("icmpv6.mld.qqi", OstProto::Gmp::kQqiFieldNumber); // FIXME
    fieldMap_.insert("icmpv6.mld.nb_sources", 
            OstProto::Gmp::kSourceCountFieldNumber);

    fieldMap_.insert("icmpv6.mldr.nb_mcast_records", 
            OstProto::Gmp::kGroupRecordCountFieldNumber);
}

PdmlProtocol* PdmlMldProtocol::createInstance()
{
    return new PdmlMldProtocol();
}

void PdmlMldProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes &attributes, int /*expectedPos*/, 
        OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Gmp *mld = pbProto->MutableExtension(OstProto::mld);

    mld->set_is_override_rsvd_code(true);
    mld->set_is_override_checksum(true);
    mld->set_is_override_source_count(true);
    mld->set_is_override_group_record_count(true);

    protoSize_ = attributes.value("size").toString().toUInt(&isOk);
}

void PdmlMldProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream* /*stream*/)
{
    bool isOk;
    OstProto::Gmp *mld = pbProto->MutableExtension(OstProto::mld);
    QString valueHexStr = attributes.value("value").toString();

    if (name == "icmpv6.type")
    {
        uint type =  valueHexStr.toUInt(&isOk, kBaseHex);

        if ((type == kMldQuery) && (protoSize_ >= 28))
            type = kMldV2Query;

        mld->set_type(type);
    }
    else if (name == "icmpv6.mld.multicast_address")
    {
        mld->mutable_group_address()->set_v6_hi(
                valueHexStr.left(16).toULongLong(&isOk, kBaseHex));
        mld->mutable_group_address()->set_v6_lo(
                valueHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mld.source_address")
    {
        OstProto::Gmp::IpAddress *ip = mld->add_sources();
        ip->set_v6_hi(valueHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip->set_v6_lo(valueHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mldr.mar.record_type")
    {
        OstProto::Gmp::GroupRecord *rec = mld->add_group_records();
        rec->set_type(OstProto::Gmp::GroupRecord::RecordType(
                    valueHexStr.toUInt(&isOk, kBaseHex)));
        rec->set_is_override_source_count(true);
        rec->set_is_override_aux_data_length(true);
    }
    else if (name == "icmpv6.mldr.mar.aux_data_len")
    {
        mld->mutable_group_records(mld->group_records_size() - 1)->
            set_aux_data_length(valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mldr.mar.nb_sources")
    {
        mld->mutable_group_records(mld->group_records_size() - 1)->
            set_source_count(valueHexStr.toUInt(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mldr.mar.multicast_address")
    {
        OstProto::Gmp::IpAddress *ip = mld->mutable_group_records(
                mld->group_records_size() - 1)->mutable_group_address();
        ip->set_v6_hi(valueHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip->set_v6_lo(valueHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mldr.mar.source_address")
    {
        OstProto::Gmp::IpAddress *ip = mld->mutable_group_records(
                mld->group_records_size() - 1)->add_sources();
        ip->set_v6_hi(valueHexStr.left(16).toULongLong(&isOk, kBaseHex));
        ip->set_v6_lo(valueHexStr.right(16).toULongLong(&isOk, kBaseHex));
    }
    else if (name == "icmpv6.mldr.mar.auxiliary_data")
    {
        QByteArray ba = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());
        mld->mutable_group_records(mld->group_records_size() - 1)->
            set_aux_data(ba.constData(), ba.size());
    }
}


// ---------------------------------------------------------- //
// PdmlTcpProtocol                                            //
// ---------------------------------------------------------- //

PdmlTcpProtocol::PdmlTcpProtocol()
{
    pdmlProtoName_ = "tcp";
    ostProtoId_ = OstProto::Protocol::kTcpFieldNumber;

    fieldMap_.insert("tcp.srcport", OstProto::Tcp::kSrcPortFieldNumber);
    fieldMap_.insert("tcp.dstport", OstProto::Tcp::kDstPortFieldNumber);
    fieldMap_.insert("tcp.seq", OstProto::Tcp::kSeqNumFieldNumber);
    fieldMap_.insert("tcp.ack", OstProto::Tcp::kAckNumFieldNumber);
    fieldMap_.insert("tcp.hdr_len", OstProto::Tcp::kHdrlenRsvdFieldNumber);
    fieldMap_.insert("tcp.flags", OstProto::Tcp::kFlagsFieldNumber);
    fieldMap_.insert("tcp.window_size", OstProto::Tcp::kWindowFieldNumber);
    fieldMap_.insert("tcp.checksum", OstProto::Tcp::kCksumFieldNumber);
    fieldMap_.insert("tcp.urgent_pointer", OstProto::Tcp::kUrgPtrFieldNumber);
}

PdmlProtocol* PdmlTcpProtocol::createInstance()
{
    return new PdmlTcpProtocol();
}

void PdmlTcpProtocol::unknownFieldHandler(QString name, int /*pos*/, 
            int /*size*/, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    if (name == "tcp.options")
        options_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
    else if (name == "")
    {
        if (attributes.value("show").toString().startsWith("TCP segment data"))
        {
            segmentData_ = QByteArray::fromHex(attributes.value("value").toString().toUtf8());
            stream->mutable_core()->mutable_name()->insert(0, 
                    segmentData_.constData(), segmentData_.size());
        }
        else if (attributes.value("show").toString().startsWith("Acknowledgement number"))
        {
            bool isOk;
            OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

            tcp->set_ack_num(attributes.value("value").toString().toUInt(&isOk, kBaseHex)); 
        }
    }
}

void PdmlTcpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::Tcp *tcp = pbProto->MutableExtension(OstProto::tcp);

    qDebug("Tcp: post\n");

    tcp->set_is_override_src_port(true); // FIXME
    tcp->set_is_override_dst_port(true); // FIXME
    tcp->set_is_override_hdrlen(true); // FIXME
    tcp->set_is_override_cksum(true); // FIXME

    if (options_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(options_.constData(), 
                options_.size());
        hexDump->set_pad_until_end(false);
        options_.resize(0);
    }

#if 0
    if (segmentData_.size())
    {
        OstProto::Protocol *proto = stream->add_protocol();

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        hexDump->mutable_content()->append(segmentData_.constData(), 
                segmentData_.size());
        hexDump->set_pad_until_end(false);
        segmentData_.resize(0);
    }
#endif
}

// ---------------------------------------------------------- //
// PdmlUdpProtocol                                            //
// ---------------------------------------------------------- //

PdmlUdpProtocol::PdmlUdpProtocol()
{
    pdmlProtoName_ = "udp"; // OR udplite
    ostProtoId_ = OstProto::Protocol::kUdpFieldNumber;

    fieldMap_.insert("udp.srcport", OstProto::Udp::kSrcPortFieldNumber);
    fieldMap_.insert("udp.dstport", OstProto::Udp::kDstPortFieldNumber);
    fieldMap_.insert("udp.length", OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum_coverage", 
                     OstProto::Udp::kTotlenFieldNumber);
    fieldMap_.insert("udp.checksum", OstProto::Udp::kCksumFieldNumber);
}

PdmlProtocol* PdmlUdpProtocol::createInstance()
{
    return new PdmlUdpProtocol();
}

void PdmlUdpProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream* /*stream*/)
{
    OstProto::Udp *udp = pbProto->MutableExtension(OstProto::udp);

    qDebug("Udp: post\n");

    udp->set_is_override_src_port(true);
    udp->set_is_override_dst_port(true);
    udp->set_is_override_totlen(true);
    udp->set_is_override_cksum(true);
}


// ---------------------------------------------------------- //
// PdmlTextProtocol                                            //
// ---------------------------------------------------------- //

PdmlTextProtocol::PdmlTextProtocol()
{
    pdmlProtoName_ = "text";
    ostProtoId_ = OstProto::Protocol::kTextProtocolFieldNumber;
}

PdmlProtocol* PdmlTextProtocol::createInstance()
{
    return new PdmlTextProtocol();
}

void PdmlTextProtocol::preProtocolHandler(QString /*name*/, 
        const QXmlStreamAttributes &attributes, int expectedPos, 
        OstProto::Protocol *pbProto, OstProto::Stream *stream)
{
    bool isOk;
    int size;
    int pos = attributes.value("pos").toString().toUInt(&isOk);

    if (!isOk)
    {
        if (expectedPos >= 0)
            expPos_ = pos = expectedPos;
        else
            goto _skip_pos_size_proc;
    }

    size = attributes.value("size").toString().toUInt(&isOk);
    if (!isOk)
        goto _skip_pos_size_proc;

    // If pos+size goes beyond the frame length, this is a "reassembled"
    // protocol and should be skipped
    if ((pos + size) > int(stream->core().frame_len()))
        goto _skip_pos_size_proc;

    expPos_ = pos;
    endPos_ = expPos_ + size;

_skip_pos_size_proc:
    qDebug("expPos_ = %d, endPos_ = %d", expPos_, endPos_);
    OstProto::TextProtocol *text = pbProto->MutableExtension(
            OstProto::textProtocol);

    text->set_port_num(0);
    text->set_eol(OstProto::TextProtocol::kCrLf); // by default we assume CRLF

    detectEol_ = true;
    contentType_ = kUnknownContent;
}

void PdmlTextProtocol::unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, OstProto::Protocol *pbProto,
            OstProto::Stream* /*stream*/)
{
_retry:
    switch(contentType_)
    {
    case kUnknownContent:
        if (name == "data")
            contentType_ = kOtherContent;
        else
            contentType_ = kTextContent;
        goto _retry;
        break;

    case kTextContent:
    {
        OstProto::TextProtocol *text = pbProto->MutableExtension(
                OstProto::textProtocol);

        if ((name == "data") 
            || (attributes.value("show") == "HTTP chunked response"))
        {
            contentType_ = kOtherContent;
            goto _retry;
        }

        if (pos < expPos_)
            break;

        if ((pos + size) > endPos_)
            break;

        if (pos > expPos_)
        {   
            int gap = pos - expPos_;
            QByteArray filler(gap, '\n');

            if (text->eol() == OstProto::TextProtocol::kCrLf)
            {
                if (gap & 0x01) // Odd
                {
                    filler.resize(gap/2 + 1);
                    filler[0]=int(' ');
                }
                else // Even
                    filler.resize(gap/2);
            }

            text->mutable_text()->append(filler.constData(), filler.size());
            expPos_ += gap;
        }

        QByteArray line = QByteArray::fromHex(
                attributes.value("value").toString().toUtf8());

        if (detectEol_)
        {
            if (line.right(2) == "\r\n")
                text->set_eol(OstProto::TextProtocol::kCrLf);
            else if (line.right(1) == "\r")
                text->set_eol(OstProto::TextProtocol::kCr);
            else if (line.right(1) == "\n")
                text->set_eol(OstProto::TextProtocol::kLf);

            detectEol_ = false;
        }

        // Convert line endings to LF only - Qt reqmt that TextProto honours
        line.replace("\r\n", "\n");
        line.replace('\r', '\n');

        text->mutable_text()->append(line.constData(), line.size());
        expPos_ += size;
        break;
    }
    case kOtherContent:
        // Do nothing!
        break;
    default:
       Q_ASSERT(false);
    }
}

void PdmlTextProtocol::postProtocolHandler(OstProto::Protocol *pbProto,
        OstProto::Stream *stream)
{
    OstProto::TextProtocol *text = pbProto->MutableExtension(
            OstProto::textProtocol);

    // Empty Text Content - remove ourselves
    if (text->text().length() == 0)
        stream->mutable_protocol()->RemoveLast();

    expPos_ = endPos_ = -1;
    detectEol_ = true;
    contentType_ = kUnknownContent;
}
