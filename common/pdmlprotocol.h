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

#ifndef _PDML_PROTOCOL_H
#define _PDML_PROTOCOL_H

#include "protocol.pb.h"

#include <google/protobuf/descriptor.h>
#include <QMap>
#include <QString>
#include <QXmlStreamAttributes>

// TODO: add const where possible

class PdmlProtocol
{
public:
    PdmlProtocol(); // TODO: make private
    virtual ~PdmlProtocol();

    static PdmlProtocol* createInstance();

    QString pdmlProtoName() const;
    int ostProtoId() const;
    bool hasField(QString name) const;
    int fieldId(QString name) const;

    virtual void preProtocolHandler(QString name, 
            const QXmlStreamAttributes &attributes, int expectedPos, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void prematureEndHandler(int pos, OstProto::Protocol *pbProto,
            OstProto::Stream *stream);
    virtual void postProtocolHandler(OstProto::Protocol *pbProto, 
            OstProto::Stream *stream);

    void fieldHandler(QString name, const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    void knownFieldHandler(QString name, QString valueHexStr,
            OstProto::Protocol *pbProto);
    virtual void unknownFieldHandler(QString name, int pos, int size, 
            const QXmlStreamAttributes &attributes, 
            OstProto::Protocol *pbProto, OstProto::Stream *stream);

protected:
    QString pdmlProtoName_; // TODO: needed? duplicated in protocolMap_
    int ostProtoId_;
    QMap<QString, int> fieldMap_;
};

#endif
