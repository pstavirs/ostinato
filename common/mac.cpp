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

#include "mac.h"

MacConfigForm::MacConfigForm(QWidget *parent)
    : QWidget(parent)
{
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

    setupUi(this);
    leDstMac->setValidator(new QRegExpValidator(reMac, this));
    leSrcMac->setValidator(new QRegExpValidator(reMac, this));
    leDstMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
    leSrcMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
}

MacConfigForm::~MacConfigForm()
{
    qDebug("In MacConfigForm destructor");
}

void MacConfigForm::on_cmbDstMacMode_currentIndexChanged(int index)
{
    if (index == OstProto::Mac::e_mm_fixed)
    {
        leDstMacCount->setEnabled(false);
        leDstMacStep->setEnabled(false);
    }
    else
    {
        leDstMacCount->setEnabled(true);
        leDstMacStep->setEnabled(true);
    }
}

void MacConfigForm::on_cmbSrcMacMode_currentIndexChanged(int index)
{
    if (index == OstProto::Mac::e_mm_fixed)
    {
        leSrcMacCount->setEnabled(false);
        leSrcMacStep->setEnabled(false);
    }
    else
    {
        leSrcMacCount->setEnabled(true);
        leSrcMacStep->setEnabled(true);
    }
}


MacProtocol::MacProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent)
{
    configForm = NULL;
}

MacProtocol::~MacProtocol()
{
    delete configForm;
}

AbstractProtocol* MacProtocol::createInstance(StreamBase *stream
    , AbstractProtocol *parent)
{
    return new MacProtocol(stream, parent);
}

quint32 MacProtocol::protocolNumber() const
{
    return OstProto::Protocol::kMacFieldNumber;
}

void MacProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::mac)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void MacProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() && 
            protocol.HasExtension(OstProto::mac))
        data.MergeFrom(protocol.GetExtension(OstProto::mac));
}

QString MacProtocol::name() const
{
    return QString("Media Access Protocol");
}

QString MacProtocol::shortName() const
{
    return QString("MAC");
}

int    MacProtocol::fieldCount() const
{
    return mac_fieldCount;
}

AbstractProtocol::FieldFlags MacProtocol::fieldFlags(int index) const
{
    AbstractProtocol::FieldFlags flags;

    flags = AbstractProtocol::fieldFlags(index);

    switch (index)
    {
        case mac_dstAddr:
        case mac_srcAddr:
            break;

        case mac_dstMacMode:
        case mac_dstMacCount:
        case mac_dstMacStep:
        case mac_srcMacMode:
        case mac_srcMacCount:
        case mac_srcMacStep:
            flags &= ~FrameField;
            flags |= MetaField;
            break;
    }

    return flags;
}

QVariant MacProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case mac_dstAddr:
        {
            int u;
            quint64 dstMac = 0;

            switch (data.dst_mac_mode())
            {
                case OstProto::Mac::e_mm_fixed:
                    dstMac = data.dst_mac();
                    break;
                case OstProto::Mac::e_mm_inc:
                    u = (streamIndex % data.dst_mac_count()) * 
                        data.dst_mac_step(); 
                    dstMac = data.dst_mac() + u;
                    break;
                case OstProto::Mac::e_mm_dec:
                    u = (streamIndex % data.dst_mac_count()) * 
                        data.dst_mac_step(); 
                    dstMac = data.dst_mac() - u;
                    break;
                default:
                    qWarning("Unhandled dstMac_mode %d", data.dst_mac_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Desination");
                case FieldValue:
                    return dstMac;
                case FieldTextValue:
                    return uintToHexStr(dstMac, 6);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian(dstMac, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        case mac_srcAddr:
        {
            int u;
            quint64 srcMac = 0;

            switch (data.src_mac_mode())
            {
                case OstProto::Mac::e_mm_fixed:
                    srcMac = data.src_mac();
                    break;
                case OstProto::Mac::e_mm_inc:
                    u = (streamIndex % data.src_mac_count()) * 
                        data.src_mac_step(); 
                    srcMac = data.src_mac() + u;
                    break;
                case OstProto::Mac::e_mm_dec:
                    u = (streamIndex % data.src_mac_count()) * 
                        data.src_mac_step(); 
                    srcMac = data.src_mac() - u;
                    break;
                default:
                    qWarning("Unhandled srcMac_mode %d", data.src_mac_mode());
            }

            switch(attrib)
            {
                case FieldName:            
                    return QString("Source");
                case FieldValue:
                    return srcMac;
                case FieldTextValue:
                    return uintToHexStr(srcMac, 6);
                case FieldFrameValue:
                {
                    QByteArray fv;
                    fv.resize(8);
                    qToBigEndian(srcMac, (uchar*) fv.data());
                    fv.remove(0, 2);
                    return fv;
                }
                default:
                    break;
            }
            break;
        }
        // Meta fields
        case mac_dstMacMode:
        case mac_dstMacCount:
        case mac_dstMacStep:
        case mac_srcMacMode:
        case mac_srcMacCount:
        case mac_srcMacStep:
        default:
            break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool MacProtocol::setFieldData(int /*index*/, const QVariant& /*value*/, 
        FieldAttrib /*attrib*/)
{
    return false;
}

bool MacProtocol::isProtocolFrameValueVariable() const
{
    if ((data.dst_mac_mode() != OstProto::Mac::e_mm_fixed) ||
        (data.src_mac_mode() != OstProto::Mac::e_mm_fixed))
        return true;
    else
        return false;
}

int MacProtocol::protocolFrameVariableCount() const
{
    int count = 1;

    if (data.dst_mac_mode() != OstProto::Mac::e_mm_fixed)
        count = AbstractProtocol::lcm(count, data.dst_mac_count());

    if (data.src_mac_mode() != OstProto::Mac::e_mm_fixed)
        count = AbstractProtocol::lcm(count, data.src_mac_count());

    return count;
}

QWidget* MacProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new MacConfigForm;
        loadConfigWidget();
    }
    return configForm;
}

void MacProtocol::loadConfigWidget()
{
    configWidget();

    configForm->leDstMac->setText(uintToHexStr(data.dst_mac(), 6));
    configForm->cmbDstMacMode->setCurrentIndex(data.dst_mac_mode());
    configForm->leDstMacCount->setText(QString().setNum(data.dst_mac_count()));
    configForm->leDstMacStep->setText(QString().setNum(data.dst_mac_step()));

    configForm->leSrcMac->setText(uintToHexStr(data.src_mac(), 6));
    configForm->cmbSrcMacMode->setCurrentIndex(data.src_mac_mode());
    configForm->leSrcMacCount->setText(QString().setNum(data.src_mac_count()));
    configForm->leSrcMacStep->setText(QString().setNum(data.src_mac_step()));
}

void MacProtocol::storeConfigWidget()
{
    bool isOk;

    configWidget();

    data.set_dst_mac(configForm->leDstMac->text().remove(QChar(' ')).
            toULongLong(&isOk, 16));
    data.set_dst_mac_mode((OstProto::Mac::MacAddrMode) configForm->
            cmbDstMacMode->currentIndex());
    data.set_dst_mac_count(configForm->leDstMacCount->text().toULong(&isOk));
    data.set_dst_mac_step(configForm->leDstMacStep->text().toULong(&isOk));

    data.set_src_mac(configForm->leSrcMac->text().remove(QChar(' ')).
            toULongLong(&isOk, 16));
    data.set_src_mac_mode((OstProto::Mac::MacAddrMode) configForm->
            cmbSrcMacMode->currentIndex());
    data.set_src_mac_count(configForm->leSrcMacCount->text().toULong(&isOk));
    data.set_src_mac_step(configForm->leSrcMacStep->text().toULong(&isOk));
}

