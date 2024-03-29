/*
Copyright (C) 2021 Srivats P.

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

#ifndef _GRE_PDML_H
#define _GRE_PDML_H

#include "pdmlprotocol.h"

class PdmlGreProtocol : public PdmlProtocol
{
public:
    virtual ~PdmlGreProtocol();

    static PdmlProtocol* createInstance();

    virtual void postProtocolHandler(OstProto::Protocol *pbProto,
            OstProto::Stream *stream);

    void fieldHandler(QString name, const QXmlStreamAttributes &attributes,
            OstProto::Protocol *pbProto, OstProto::Stream *stream);
    virtual void unknownFieldHandler(QString name, int pos, int size,
            const QXmlStreamAttributes &attributes,
            OstProto::Protocol *pbProto, OstProto::Stream *stream);

protected:
    PdmlGreProtocol();
};

#endif
