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

#include "portconfigdialog.h"

PortConfigDialog::PortConfigDialog(OstProto::Port &portConfig, QWidget *parent)
        : QDialog(parent), portConfig_(portConfig)
{
    qDebug("In %s", __FUNCTION__);

    setupUi(this);

    switch(portConfig_.transmit_mode())
    {
    case OstProto::kSequentialTransmit:
        sequentialStreamsButton->setChecked(true);
        break;
    case OstProto::kInterleavedTransmit:
        interleavedStreamsButton->setChecked(true);
        break;
    default:
        Q_ASSERT(false); // Unreachable!!!
        break;
    }

    exclusiveControlButton->setChecked(portConfig_.is_exclusive_control());
}

void PortConfigDialog::accept()
{
    if (sequentialStreamsButton->isChecked())
        portConfig_.set_transmit_mode(OstProto::kSequentialTransmit);
    else if (interleavedStreamsButton->isChecked())
        portConfig_.set_transmit_mode(OstProto::kInterleavedTransmit);
    else
        Q_ASSERT(false); // Unreachable!!!

    portConfig_.set_is_exclusive_control(exclusiveControlButton->isChecked());

    QDialog::accept();
}
