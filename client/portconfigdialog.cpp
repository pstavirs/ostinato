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
#include "settings.h"

PortConfigDialog::PortConfigDialog(OstProto::Port &portConfig, QWidget *parent)
        : QDialog(parent), portConfig_(portConfig)
{
    QString currentUser(portConfig_.user_name().c_str());

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

    // Port Reservation
    myself_ = appSettings->value(kUserKey, kUserDefaultValue).toString();
    // XXX: what if myself_ is empty?
    if (currentUser.isEmpty()) {
        reservedBy_ = kNone;
        reservedBy->setText("Unreserved");
        reserveButton->setText("Reserve");
    }
    else if (currentUser == myself_) {
        reservedBy_ = kSelf;
        reservedBy->setText("Reserved by: me <i>("+currentUser+")</i>");
        reserveButton->setText("Reserve (uncheck to unreserve)");
        reserveButton->setChecked(true);
    }
    else {
        reservedBy_ = kOther;
        reservedBy->setText("Reserved by: <i>"+currentUser+"</i>");
        reserveButton->setText("Force reserve");
    }
    qDebug("reservedBy_ = %d", reservedBy_);

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

    switch (reservedBy_) {
        case kSelf:
            if (!reserveButton->isChecked())
                portConfig_.set_user_name(""); // unreserve
            break;

        case kOther:
        case kNone:
            if (reserveButton->isChecked())
                portConfig_.set_user_name(
                        myself_.toStdString()); // (force) reserve
            break;

        default:
            qWarning("Unreachable code");
            break;
    }

    portConfig_.set_is_exclusive_control(exclusiveControlButton->isChecked());

    QDialog::accept();
}
