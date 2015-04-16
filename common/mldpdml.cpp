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

#include "mldpdml.h"

#include "mld.pb.h"

PdmlMldProtocol::PdmlMldProtocol()
{
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

