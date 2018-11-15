/*
Copyright (C) 2015 Srivats P.

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

#ifndef _VARIABLE_FIELDS_WIDGET_H
#define _VARIABLE_FIELDS_WIDGET_H

#include "protocol.pb.h"
#include "ui_variablefieldswidget.h"

#include <QWidget>

class AbstractProtocol;
class Stream;
class QListWidgetItem;

class VariableFieldsWidget : public QWidget, private Ui::VariableFieldsWidget
{
    Q_OBJECT
public:
    VariableFieldsWidget(QWidget *parent = 0);

    void setStream(Stream *stream);

    void load();
    void store();
    void clear();

private slots:
    void on_protocolList_currentItemChanged(
            QListWidgetItem *current, 
            QListWidgetItem *previous);
    void on_variableFieldList_currentItemChanged(
            QListWidgetItem *current, 
            QListWidgetItem *previous);
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_field_currentIndexChanged(int index);
    void on_type_currentIndexChanged(int index);
    void updateCurrentVariableField();
private:
    void decorateProtocolItem(QListWidgetItem *item);
    void loadProtocolFields(const AbstractProtocol *protocol);
    int typeSize(OstProto::VariableField::Type type);
    int fieldIndex(const OstProto::VariableField &vf);
    void setVariableFieldItem(
            QListWidgetItem *item,
            const AbstractProtocol *protocol,
            const OstProto::VariableField &vf);

    // Custom roles for protocol items
    enum {
        kProtocolPtrRole = Qt::UserRole,
        kCurrentVarFieldRole,
    };

    // Custom roles for variable field items
    enum {
        kVarFieldRole = Qt::UserRole
    };

    Stream *stream_;
    QIntValidator *valueRange_;
    bool isProgLoad_;
    // FIXME: make the lastXXX vars static?
    int lastSelectedProtocolIndex_;
    int lastSelectedVariableFieldIndex_;
};
#endif
