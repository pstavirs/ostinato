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

#include "textprotopdml.h"

#include "textproto.pb.h"

PdmlTextProtocol::PdmlTextProtocol()
{
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
