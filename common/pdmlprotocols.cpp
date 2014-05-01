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

#include "pdmlprotocols.h"

#include "hexdump.pb.h"

// ---------------------------------------------------------- //
// PdmlUnknownProtocol                                        //
// ---------------------------------------------------------- //

PdmlUnknownProtocol::PdmlUnknownProtocol()
{
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
}

PdmlProtocol* PdmlGenInfoProtocol::createInstance()
{
    return new PdmlGenInfoProtocol();
}

// ---------------------------------------------------------- //
// PdmlFrameProtocol                                          //
// ---------------------------------------------------------- //

PdmlFrameProtocol::PdmlFrameProtocol()
{
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
