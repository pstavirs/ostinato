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

#include "igmppdml.h"

#include "igmp.pb.h"

PdmlIgmpProtocol::PdmlIgmpProtocol()
{
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

