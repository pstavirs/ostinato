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

#include "pcapoptionsdialog.h"

PcapImportOptionsDialog::PcapImportOptionsDialog(QVariantMap *options)
    : QDialog(NULL)
{
    Q_ASSERT(options != NULL);

    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    options_ = options;

    viaPdml->setChecked(options_->value("ViaPdml").toBool());
    // XXX: By default this key is absent - so that pcap import tests
    // evaluate to false and hence show minimal diffs.
    // However, for the GUI user, this should be enabled by default.
    recalculateCksums->setChecked(
                        options_->value("RecalculateCksums", QVariant(true))
                                    .toBool());
    doDiff->setChecked(options_->value("DoDiff").toBool());

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

PcapImportOptionsDialog::~PcapImportOptionsDialog()
{
}

void PcapImportOptionsDialog::accept()
{
    options_->insert("ViaPdml", viaPdml->isChecked());
    options_->insert("RecalculateCksums", recalculateCksums->isChecked());
    options_->insert("DoDiff", doDiff->isChecked());

    QDialog::accept();
}

