/*
Copyright (C) 2014 PLVision.

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

This module is developed by PLVision  <developers@plvision.eu>
*/

#ifndef _STP_H
#define _STP_H

#include "abstractprotocol.h"
#include "stp.pb.h"

/*
Stp Protocol Frame Format -
  +------------------------------------+------------------+------------------+
  |              Protocol ID           |   Protocol VID   |     BPDU type    |
  |                 (16)               |         (8)      |      (8)         |
  +------------------+-----------------+------------------+------------------+
  |     Flags        |                    Root Identifier                  ->|
  |      (8)         |                         (64)                        ->|
  +------------------+-------------------------------------------------------+
  |->                              Root Identifier                         ->|
  |->                                (64)                                  ->|
  +------------------+-------------------------------------------------------+
  |-> Root Identifier|                       Root Path Cost                ->|
  |->    (8)         |                         (32)                        ->|
  +------------------+-------------------------------------------------------+
  |-> Root Path Cost |                       Bridge Identifier             ->|
  |->    (32)        |                         (64)                        ->|
  +------------------+-------------------------------------------------------+
  |->                             Bridge Identifier                        ->|
  |->                                 (64)                                 ->|
  +------------------+--------------------------------- --+------------------+
  |-> Bridge Identif.|           Port Identifier          |   Message Age  ->|
  |->    (64)        |                (16)                |      (16)      ->|
  +------------------+------------------------------------+------------------+
  |->  Message Age   |               Max Age              |     Hello Time ->|
  |->    (16)        |                (16)                |      (16)      ->|
  +------------------+------------------------------------+------------------+
  |->  Hello Time    |            Forward delay           |
  |->    (16)        |                (16)                |
  +------------------+------------------------------------+
Figures in brackets represent field width in bits
*/

class StpProtocol : public AbstractProtocol
{
public:
    enum stpfield
    {
        stp_protocol_id = 0,
        stp_version_id,
        stp_bpdu_type,
        stp_flags,
        stp_root_id,
        stp_root_path_cost,
        stp_bridge_id,
        stp_port_id,
        stp_message_age,
        stp_max_age,
        stp_hello_time,
        stp_forward_delay,

        stp_fieldCount
    };

    StpProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~StpProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
                                            AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

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
    OstProto::Stp    data_;
};

#endif
