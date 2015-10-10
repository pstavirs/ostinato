/*
Copyright (C) 2014 Srivats P.
Copyright (C) 2015 Ilya Volchanetskiy

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

#include "mplspdml.h"

#include "mpls.pb.h"

PdmlMplsProtocol::PdmlMplsProtocol()
{
    ostProtoId_ = OstProto::Protocol::kMplsFieldNumber;

    fieldMap_.insert("mpls.label", OstProto::Mpls::kLabelFieldNumber);
    fieldMap_.insert("mpls.exp", OstProto::Mpls::kExpFieldNumber);
    fieldMap_.insert("mpls.bottom", OstProto::Mpls::kBosFieldNumber);
    fieldMap_.insert("mpls.ttl", OstProto::Mpls::kTtlFieldNumber);
}

PdmlMplsProtocol::~PdmlMplsProtocol()
{
}

PdmlProtocol* PdmlMplsProtocol::createInstance()
{
    return new PdmlMplsProtocol();
}
