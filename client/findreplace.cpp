/*
Copyright (C) 2021 Srivats P.

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

#include "findreplace.h"

#include "../common/protocolmanager.h"

extern ProtocolManager *OstProtocolManager;

FindReplaceDialog::FindReplaceDialog(Action *action, QWidget *parent)
    : QDialog(parent), action_(action)
{
    setupUi(this);

    // Keep things simple and don't use mask(s) (default)
    useFindMask->setChecked(false);
    useReplaceMask->setChecked(false);

    protocol->addItems(OstProtocolManager->protocolDatabase());
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    protocol->setPlaceholderText(tr("Select"));
#endif
    protocol->setCurrentIndex(-1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    field->setPlaceholderText(tr("Select"));
#endif

    // Enable this setting if we have streams selected on input
    selectedStreamsOnly->setEnabled(action->selectedStreamsOnly);

    // Reset for user input
    action->selectedStreamsOnly = false;
}

void FindReplaceDialog::on_protocol_currentIndexChanged(const QString &/*name*/)
{
}
