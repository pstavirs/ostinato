/*
Copyright (C) 2019 Srivats P.

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

#ifndef _APPLY_MESSAGE_H
#define _APPLY_MESSAGE_H

#include <QDialog>
#include <QMainWindow>
#include <QProgressBar>
#include <QTimer>

extern QMainWindow *mainWindow;

class ApplyMessage: public QDialog
{
public:
    ApplyMessage(QWidget *parent = Q_NULLPTR);

public slots:
    void show();
    virtual void done(int r);

private:
    QLabel *help_;
    QTimer *helpTimer_;
};

ApplyMessage::ApplyMessage(QWidget *parent)
    : QDialog(parent)
{
    auto layout = new QVBoxLayout(this);
    auto message = new QLabel(tr("Pushing configuration changes to agent "
                              "and re-building packets ..."), this);
    auto progress = new QProgressBar(this);
    progress->setRange(0, 0);
    progress->setTextVisible(false);
    help_ = new QLabel(tr("<b>This may take some time</b>"), this);
    help_->setAlignment(Qt::AlignCenter);
    layout->addWidget(message);
    layout->addWidget(progress);
    layout->addWidget(help_);

    helpTimer_ = new QTimer(this);
    helpTimer_->setSingleShot(true);
    helpTimer_->setInterval(4000);
    connect(helpTimer_, SIGNAL(timeout()), help_, SLOT(show()));
}

void ApplyMessage::show()
{
    help_->hide(); // shown when helpTimer_ expires

    QWidget *parent = parentWidget();
    if (!parent)
        parent = mainWindow;
    move(parent->frameGeometry().center() - rect().center());
    setModal(true);
    QDialog::show();

    helpTimer_->start();
}

void ApplyMessage::done(int r)
{
    helpTimer_->stop();
    QDialog::done(r);
}

#endif

