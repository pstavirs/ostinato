/*
Copyright (C) 2010, 2014 Srivats P.

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

#include "userscriptconfig.h"
#include "userscript.h"

#include <QMessageBox>

UserScriptConfigForm::UserScriptConfigForm(QWidget *parent) 
    : AbstractProtocolConfigForm(parent)
{
    setupUi(this);

    // The protocol_ (UserScriptProtocol) is a dummy protocol internal
    // to UserScriptConfigForm whose sole purpose is to "compile" the script
    // so that the configForm widget can display the compilation result.
    // It is *not* used for actual packet contents at any time
    protocol_ = new UserScriptProtocol(NULL);
    compileScript();
}

UserScriptConfigForm::~UserScriptConfigForm() 
{
    delete protocol_;
}

UserScriptConfigForm* UserScriptConfigForm::createInstance()
{
    return new UserScriptConfigForm;
}

void UserScriptConfigForm::loadWidget(AbstractProtocol *proto)
{
    programEdit->setPlainText(
        proto->fieldData(
            UserScriptProtocol::userScript_program, 
            AbstractProtocol::FieldValue
        ).toString());

    compileScript();
}

void UserScriptConfigForm::storeWidget(AbstractProtocol *proto)
{
    proto->setFieldData(
            UserScriptProtocol::userScript_program,
            programEdit->toPlainText());
}

//
// ----- private methods
//
void UserScriptConfigForm::compileScript()
{
    // storeWidget() will save the updated userscript into
    // the protocol_ which in turn triggers the protocol_ to
    // compile it
    storeWidget(protocol_);
    if (protocol_->isScriptValid())
    {
        statusLabel->setText(QString("<font color=\"green\">Success</font>"));
        compileButton->setDisabled(true);
    }
    else
    {
        statusLabel->setText(
                QString("<font color=\"red\">Error: %1: %2</font>").arg(
                protocol_->userScriptErrorLineNumber()).arg(
                protocol_->userScriptErrorText()));
        compileButton->setEnabled(true);
    }
}

//
// ----- private slots
//
void UserScriptConfigForm::on_programEdit_textChanged()
{
    compileButton->setEnabled(true);
}

void UserScriptConfigForm::on_compileButton_clicked(bool /*checked*/)
{
    compileScript();
    if (!protocol_->isScriptValid())
    {
        QMessageBox::critical(this, "Error", 
            QString("%1: %2").arg(
                protocol_->userScriptErrorLineNumber()).arg(
                protocol_->userScriptErrorText()));
    }
}

