/*
Copyright (C) 2010 Srivats P.

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

#include <qendian.h>
#include <QHostAddress>

//#include "../client/stream.h"
#include "payload.h"
#include "streambase.h"


PayloadConfigForm::PayloadConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

void PayloadConfigForm::on_cmbPatternMode_currentIndexChanged(int index)
{
    switch(index)
    {
        case OstProto::Payload::e_dp_fixed_word:
            lePattern->setEnabled(true);
            break;
        case OstProto::Payload::e_dp_inc_byte:
        case OstProto::Payload::e_dp_dec_byte:
        case OstProto::Payload::e_dp_random:
            lePattern->setDisabled(true);
            break;
        default:
            qWarning("Unhandled/Unknown PatternMode = %d",index);
    }
}

PayloadProtocol::PayloadProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

PayloadProtocol::~PayloadProtocol()
{
    delete configForm;
}

AbstractProtocol* PayloadProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new PayloadProtocol(stream, parent);
}

quint32 PayloadProtocol::protocolNumber() const
{
    return OstProto::Protocol::kPayloadFieldNumber;
}

void PayloadProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::payload)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void PayloadProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::payload))
        data.MergeFrom(protocol.GetExtension(OstProto::payload));
}

QString PayloadProtocol::name() const
{
    return QString("Payload Data");
}

QString PayloadProtocol::shortName() const
{
    return QString("DATA");
}

int PayloadProtocol::protocolFrameSize(int streamIndex) const
{
    int len;

    len = mpStream->frameLen(streamIndex) - protocolFrameOffset(streamIndex) 
        - kFcsSize;

    if (len < 0)
        len = 0;

    qDebug("%s: this = %p, streamIndex = %d, len = %d", __FUNCTION__, this,
            streamIndex, len);
    return len;
}

int PayloadProtocol::fieldCount() const
{
    return payload_fieldCount;
}

AbstractProtocol::FieldFlags PayloadProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case payload_dataPattern:
            break;

        // Meta fields
        case payload_dataPatternMode:
            flags &= ~FrameField;
            flags |= MetaField;
            break;
    }

    return flags;
}

QVariant PayloadProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case payload_dataPattern:
            switch(attrib)
            {
                case FieldName:            
                    return QString("Data");
                case FieldValue:
                    return data.pattern();
                case FieldTextValue:
                    return QString(fieldData(index, FieldFrameValue, 
                            streamIndex).toByteArray().toHex());
                case FieldFrameValue:
                {
                    QByteArray fv;
                    int dataLen;

                    dataLen = protocolFrameSize(streamIndex);

                    // FIXME: Hack! Bad! Bad! Very Bad!!!
                    if (dataLen <= 0)
                        dataLen = 1;

                    fv.resize(dataLen+4);
                    switch(data.pattern_mode())
                    {
                        case OstProto::Payload::e_dp_fixed_word:
                            for (int i = 0; i < (dataLen/4)+1; i++)
                                qToBigEndian((quint32) data.pattern(), 
                                    (uchar*)(fv.data()+(i*4)) );
                            break;
                        case OstProto::Payload::e_dp_inc_byte:
                            for (int i = 0; i < dataLen; i++)
                                fv[i] = i % (0xFF + 1);
                            break;
                        case OstProto::Payload::e_dp_dec_byte:
                            for (int i = 0; i < dataLen; i++)
                                fv[i] = 0xFF - (i % (0xFF + 1));
                            break;
                        case OstProto::Payload::e_dp_random:
                            //! \todo (HIGH) cksum is incorrect for random pattern
                            for (int i = 0; i < dataLen; i++)
                                fv[i] =  qrand() % (0xFF + 1);
                            break;
                        default:
                            qWarning("Unhandled data pattern %d", 
                                data.pattern_mode());
                    }
                    fv.resize(dataLen);
                    return fv;
                }
                default:
                    break;
            }
            break;

        // Meta fields

        case payload_dataPatternMode:
        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool PayloadProtocol::setFieldData(int /*index*/, const QVariant &/*value*/, 
        FieldAttrib /*attrib*/)
{
    return false;
}

bool PayloadProtocol::isProtocolFrameValueVariable() const
{
    if (isProtocolFrameSizeVariable() 
            || data.pattern_mode() == OstProto::Payload::e_dp_random)
        return true;
    else
        return false;
}

bool PayloadProtocol::isProtocolFrameSizeVariable() const
{
    if (mpStream->lenMode() == StreamBase::e_fl_fixed)
        return false;
    else
        return true;
}

int PayloadProtocol::protocolFrameVariableCount() const
{
    int count = 1;

    if (data.pattern_mode() == OstProto::Payload::e_dp_random)
    {
        switch(mpStream->sendUnit())
        {
        case OstProto::StreamControl::e_su_packets:
            return mpStream->numPackets();

        case OstProto::StreamControl::e_su_bursts:
            return int(mpStream->numBursts() 
                    * mpStream->burstSize() 
                    * mpStream->burstRate());
        }
    }

    if (mpStream->lenMode() != StreamBase::e_fl_fixed)
    {
        count = AbstractProtocol::lcm(count, 
                mpStream->frameLenMax() - mpStream->frameLenMin() + 1);
    }

    return count;
}

QWidget* PayloadProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new PayloadConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void PayloadProtocol::loadConfigWidget()
{
    configWidget();

    configForm->cmbPatternMode->setCurrentIndex(data.pattern_mode());
    configForm->lePattern->setText(uintToHexStr(data.pattern(), 4));
}

void PayloadProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    data.set_pattern_mode((OstProto::Payload::DataPatternMode) 
        configForm->cmbPatternMode->currentIndex());
    data.set_pattern(configForm->lePattern->text().remove(QChar(' ')).toULong(&isOk, 16));
}

