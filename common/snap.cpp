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

#include "snap.h"

SnapConfigForm::SnapConfigForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

SnapProtocol::SnapProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

SnapProtocol::~SnapProtocol()
{
    delete configForm;
}

AbstractProtocol* SnapProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new SnapProtocol(stream, parent);
}

quint32 SnapProtocol::protocolNumber() const
{
    return OstProto::Protocol::kSnapFieldNumber;
}

void SnapProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::snap)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void SnapProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::snap))
        data.MergeFrom(protocol.GetExtension(OstProto::snap));
}

QString SnapProtocol::name() const
{
    return QString("SubNetwork Access Protocol");
}

QString SnapProtocol::shortName() const
{
    return QString("SNAP");
}

AbstractProtocol::ProtocolIdType SnapProtocol::protocolIdType() const
{
    return ProtocolIdEth;
}

quint32 SnapProtocol::protocolId(ProtocolIdType type) const
{
    switch(type)
    {
        case ProtocolIdLlc: return 0xAAAA03;
        default: break;
    }

    return AbstractProtocol::protocolId(type);
}

int    SnapProtocol::fieldCount() const
{
    return snap_fieldCount;
}

QVariant SnapProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case snap_oui:
            switch(attrib)
            {
                case FieldName:            
                    return QString("OUI");
                case FieldValue:
                    return data.oui();
                case FieldTextValue:
                    return QString("%1").arg(data.oui(), 6, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(4);
                    qToBigEndian((quint32) data.oui(), (uchar*) fv.data());
                    fv.remove(0, 1);
                    return fv;
                }
                default:
                    break;
            }
            break;
        case snap_type:
        {
            quint16 type;

            switch(attrib)
            {
                case FieldName:            
                    return QString("Type");
                case FieldValue:
                    type = payloadProtocolId(ProtocolIdEth);
                    return type;
                case FieldTextValue:
                    type = payloadProtocolId(ProtocolIdEth);
                    return QString("%1").arg(type, 4, BASE_HEX, QChar('0'));
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(2);
                    type = payloadProtocolId(ProtocolIdEth);
                    qToBigEndian(type, (uchar*) fv.data());
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool SnapProtocol::setFieldData(int /*index*/, const QVariant &/*value*/, 
        FieldAttrib /*attrib*/)
{
    return false;
}


QWidget* SnapProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new SnapConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void SnapProtocol::loadConfigWidget()
{
    configWidget();

    configForm->leOui->setText(uintToHexStr(
        fieldData(snap_oui, FieldValue).toUInt(), 3));
    configForm->leType->setText(uintToHexStr(
        fieldData(snap_type, FieldValue).toUInt(), 2));
}

void SnapProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    data.set_oui(configForm->leOui->text().toULong(&isOk, BASE_HEX));
    data.set_type(configForm->leType->text().toULong(&isOk, BASE_HEX));
}

