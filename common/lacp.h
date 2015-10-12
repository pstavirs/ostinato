/*
Copyright (C) 2014 Marchuk S.

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

This module is developed by PLVision <developers@plvision.eu> company
*/

#ifndef _LACP_H
#define _LACP_H

#include "abstractprotocol.h"
#include "lacp.pb.h"

/*
Lacp Protocol Frame Format -
  +---------+----------------+-------------+----------------+----------------+----------------+
  | Subtype | Version Number |  Actor TLV  |   Partner TLV  |  Collector TLV | Terminator TLV |
  |   (1)   |       (1)      |     (22)    |      (22)      |      (16)      |      (16)      |
  +---------+----------------+-------------+----------------+----------------+----------------+
    Actor/Partner TLV
    +------------+--------+-------------------+----------+-------+---------------+--------+--------+----------+
    |  TLV type  | Lenght |  System priority  |  System  |  Key  | Port proirity |  Port  |  State | Reserved |
    |    (1)     |  (1)   |        (2)        |   (6)    |  (2)  |     (2)       |   (2)  |   (1)  |    (3)   |
    +------------+--------+-------------------+----------+-------+---------------+--------+--------+----------+
    Collector TLV
    +------------+--------+------------+----------+
    |  TLV type  | Lenght |  MaxDelay  | Reserved |
    |     (1)    |  (1)   |    (2)     |   (12)   |
    +------------+--------+------------+----------+
    Terminator TLV
    +------------+--------+------------+----------+
    |  TLV type  | Lenght |  MaxDelay  | Reserved |
    |    (1)     |  (1)   |    (2)     |   (50)   |
    +------------+--------+------------+----------+
Figures in brackets represent field width in bytes
*/


class LacpProtocol : public AbstractProtocol
{
public:
    enum lacpfield
    {
        // Frame Fields
        lacp_subtype = 0,
        lacp_version_number,

        lacp_tlv_type_actor = 2,
        lacp_actor_length,
        lacp_actor_system_priority,
        lacp_actor_system,
        lacp_actor_key,
        lacp_actor_port_priority,
        lacp_actor_port,
        lacp_actor_state,
        lacp_actor_reserved,

        lacp_tlv_type_partner = 11,
        lacp_partner_length,
        lacp_partner_system_priority,
        lacp_partner_system,
        lacp_partner_key,
        lacp_partner_port_priority,
        lacp_partner_port,
        lacp_partner_state,
        lacp_partner_reserved,

        lacp_tlv_type_collector = 20,
        lacp_collector_length,
        lacp_collector_maxDelay,
        lacp_collector_reserved,
        lacp_tlv_type_terminator,
        lacp_terminator_length,
        lacp_terminator_reserved,

       // Meta Fields
        is_override_subtype = 27,
        is_override_version,
        is_override_tlv_actor,
        is_override_actor_info,
        is_override_actor_reserved,
        is_override_tlv_partner,
        is_override_partner_info ,
        is_override_partner_reserved,
        is_override_tlv_collector,
        is_override_collector_info,
        is_override_collector_reserved,
        is_override_tlv_terminator,
        is_override_terminator_info,
        is_override_terminator_reserved,

        lacp_fieldCount
    };

    LacpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~LacpProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual ProtocolIdType protocolIdType() const;
    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
    virtual QVariant fieldData(int index, FieldAttrib attrib,
               int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value,
            FieldAttrib attrib = FieldValue);

private:
    OstProto::Lacp    data;
};

#endif
