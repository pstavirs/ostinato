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

#include "tcpconfig.h"
#include "tcp.h"

TcpConfigForm::TcpConfigForm(QWidget *parent)
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);
}

TcpConfigForm::~TcpConfigForm()
{
}

TcpConfigForm* TcpConfigForm::createInstance()
{
    return new TcpConfigForm;
}

void TcpConfigForm::loadWidget(AbstractProtocol *proto)
{
    leTcpSrcPort->setText(
            proto->fieldData(
                TcpProtocol::tcp_src_port, 
                AbstractProtocol::FieldValue
            ).toString());
    cbTcpSrcPortOverride->setChecked(
            proto->fieldData(
                TcpProtocol::tcp_is_override_src_port, 
                AbstractProtocol::FieldValue
            ).toBool());

    leTcpDstPort->setText(
            proto->fieldData(
                TcpProtocol::tcp_dst_port, 
                AbstractProtocol::FieldValue
            ).toString());
    cbTcpDstPortOverride->setChecked(
            proto->fieldData(
                TcpProtocol::tcp_is_override_dst_port, 
                AbstractProtocol::FieldValue
            ).toBool());

    leTcpSeqNum->setText(
            proto->fieldData(
                TcpProtocol::tcp_seq_num, 
                AbstractProtocol::FieldValue
            ).toString());
    leTcpAckNum->setText(
            proto->fieldData(
                TcpProtocol::tcp_ack_num, 
                AbstractProtocol::FieldValue
            ).toString());

    leTcpHdrLen->setText(
            proto->fieldData(
                TcpProtocol::tcp_hdrlen, 
                AbstractProtocol::FieldValue
            ).toString());
    cbTcpHdrLenOverride->setChecked(
            proto->fieldData(
                TcpProtocol::tcp_is_override_hdrlen, 
                AbstractProtocol::FieldValue
            ).toBool());

    leTcpWindow->setText(
            proto->fieldData(
                TcpProtocol::tcp_window, 
                AbstractProtocol::FieldValue
            ).toString());

    leTcpCksum->setText(uintToHexStr(
                proto->fieldData(
                    TcpProtocol::tcp_cksum, 
                    AbstractProtocol::FieldValue
                ).toUInt(), 2));
    cbTcpCksumOverride->setChecked(
            proto->fieldData(
                TcpProtocol::tcp_is_override_cksum, 
                AbstractProtocol::FieldValue
                ).toBool());

    leTcpUrgentPointer->setText(
            proto->fieldData(
                TcpProtocol::tcp_urg_ptr, 
                AbstractProtocol::FieldValue
            ).toString());

    uint flags = proto->fieldData(
                            TcpProtocol::tcp_flags, 
                            AbstractProtocol::FieldValue
                        ).toUInt();

    cbTcpFlagsUrg->setChecked((flags & TCP_FLAG_URG) > 0);
    cbTcpFlagsAck->setChecked((flags & TCP_FLAG_ACK) > 0);
    cbTcpFlagsPsh->setChecked((flags & TCP_FLAG_PSH) > 0);
    cbTcpFlagsRst->setChecked((flags & TCP_FLAG_RST) > 0);
    cbTcpFlagsSyn->setChecked((flags & TCP_FLAG_SYN) > 0);
    cbTcpFlagsFin->setChecked((flags & TCP_FLAG_FIN) > 0);
}

void TcpConfigForm::storeWidget(AbstractProtocol *proto)
{
    int ff = 0;

    proto->setFieldData(
            TcpProtocol::tcp_src_port,
            leTcpSrcPort->text());
    proto->setFieldData(
            TcpProtocol::tcp_is_override_src_port, 
            cbTcpSrcPortOverride->isChecked());
    proto->setFieldData(
            TcpProtocol::tcp_dst_port,
            leTcpDstPort->text());
    proto->setFieldData(
            TcpProtocol::tcp_is_override_dst_port, 
            cbTcpDstPortOverride->isChecked());

    proto->setFieldData(
            TcpProtocol::tcp_seq_num,
            leTcpSeqNum->text());
    proto->setFieldData(
            TcpProtocol::tcp_ack_num,
            leTcpAckNum->text());

    proto->setFieldData(
            TcpProtocol::tcp_hdrlen,
            leTcpHdrLen->text());
    proto->setFieldData(
            TcpProtocol::tcp_is_override_hdrlen, 
            cbTcpHdrLenOverride->isChecked());

    proto->setFieldData(
            TcpProtocol::tcp_window,
            leTcpWindow->text());

    proto->setFieldData(
            TcpProtocol::tcp_cksum,
            hexStrToUInt(leTcpCksum->text()));
    proto->setFieldData(
            TcpProtocol::tcp_is_override_cksum, 
            cbTcpCksumOverride->isChecked());

    proto->setFieldData(
            TcpProtocol::tcp_urg_ptr,
            leTcpUrgentPointer->text());

    if (cbTcpFlagsUrg->isChecked()) ff |= TCP_FLAG_URG;
    if (cbTcpFlagsAck->isChecked()) ff |= TCP_FLAG_ACK;
    if (cbTcpFlagsPsh->isChecked()) ff |= TCP_FLAG_PSH;
    if (cbTcpFlagsRst->isChecked()) ff |= TCP_FLAG_RST;
    if (cbTcpFlagsSyn->isChecked()) ff |= TCP_FLAG_SYN;
    if (cbTcpFlagsFin->isChecked()) ff |= TCP_FLAG_FIN;

    proto->setFieldData(TcpProtocol::tcp_flags, ff);
}

