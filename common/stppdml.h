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

#ifndef _STP_PDML_H
#define _STP_PDML_H

#include "pdmlprotocol.h"

class PdmlStpProtocol : public PdmlProtocol
{
public:
    virtual ~PdmlStpProtocol();

    static PdmlProtocol *createInstance();

    void preProtocolHandler(QString name,
                            const QXmlStreamAttributes& attributes,
                            int expectedPos, OstProto::Protocol* pbProto,
                            OstProto::Stream* stream);
    void unknownFieldHandler(QString name, int pos, int size,
                             const QXmlStreamAttributes &attributes,
                             OstProto::Protocol *pbProto, OstProto::Stream* stream);

protected:
    PdmlStpProtocol();

private:
    int relativePos(int pos) { return pos - _protoStartPos; }
    int _protoStartPos{0};
};

#endif
