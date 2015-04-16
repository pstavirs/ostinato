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

#ifndef _GMP_CONFIG_H
#define _GMP_CONFIG_H

#include "abstractprotocolconfig.h"
#include "ui_gmp.h"

class GmpConfigForm : 
    public AbstractProtocolConfigForm,
    protected Ui::Gmp
{
    Q_OBJECT
public:
    GmpConfigForm(QWidget *parent = 0);
    ~GmpConfigForm();

    virtual void loadWidget(AbstractProtocol *proto);
    virtual void storeWidget(AbstractProtocol *proto);

protected:
    QString _defaultGroupIp;
    QString _defaultSourceIp;
    enum {
        kSsmQueryPage = 0,
        kSsmReportPage = 1
    };

private:
    void update();

private slots:
    void on_groupMode_currentIndexChanged(int index);
    void on_addSource_clicked();
    void on_deleteSource_clicked();

    void on_addGroupRecord_clicked();
    void on_deleteGroupRecord_clicked();
    void on_groupList_currentItemChanged(QListWidgetItem *current,
            QListWidgetItem *previous);
    void on_addGroupRecordSource_clicked();
    void on_deleteGroupRecordSource_clicked();
    void on_auxData_textChanged(const QString &text);
};

#endif
