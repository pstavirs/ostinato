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

#ifndef _PORTS_WINDOW_H
#define _PORTS_WINDOW_H

#include <QWidget>
#include <QAbstractItemModel>
#include "ui_portswindow.h"
#include "portgrouplist.h"

class ApplyMessage;
class QAbstractItemDelegate;
class QProgressDialog;
class QSortFilterProxyModel;

namespace OstProto {
    class SessionContent;
}

class PortsWindow : public QWidget, private Ui::PortsWindow
{
    Q_OBJECT

    //QAbstractItemModel    *slm; // stream list model
    PortGroupList        *plm;

public:
    PortsWindow(PortGroupList *pgl, QWidget *parent = 0);
    ~PortsWindow();

    int portGroupCount();
    int reservedPortCount();

    bool openSession(const OstProto::SessionContent *session,
                     QString &error);
    bool saveSession(OstProto::SessionContent *session,
                     QString &error,
                     QProgressDialog *progress = NULL);

signals:
    void currentPortChanged(const QModelIndex &current,
                            const QModelIndex &previous);

private:
    QString        lastNewPortGroup;
    QAbstractItemDelegate *delegate;
    QSortFilterProxyModel *proxyPortModel;
    ApplyMessage *applyMsg_;

public slots:
    void showMyReservedPortsOnly(bool enabled);

private slots:
    void updateApplyHint(int portGroupId, int portId, bool configChanged);
    void updatePortViewActions(const QModelIndex& currentIndex);
    void updateStreamViewActions();

    void on_startTx_clicked();
    void on_stopTx_clicked();
    void on_averagePacketsPerSec_editingFinished();
    void on_averageBitsPerSec_editingFinished();
    void updatePortRates();
    void on_tvStreamList_activated(const QModelIndex & index);
    void when_portView_currentChanged(const QModelIndex& currentIndex,
        const QModelIndex& previousIndex);
    void when_portModel_dataChanged(const QModelIndex& topLeft,
        const QModelIndex& bottomRight);
    void when_portModel_reset();

    void on_pbApply_clicked();    

    void on_actionNew_Port_Group_triggered();
    void on_actionDelete_Port_Group_triggered();
    void on_actionConnect_Port_Group_triggered();
    void on_actionDisconnect_Port_Group_triggered();

    void on_actionExclusive_Control_triggered(bool checked);
    void on_actionPort_Configuration_triggered();

    void on_actionNew_Stream_triggered();
    void on_actionEdit_Stream_triggered();
    void on_actionDuplicate_Stream_triggered();
    void on_actionDelete_Stream_triggered();

    void on_actionOpen_Streams_triggered();
    void on_actionSave_Streams_triggered();

    void streamModelDataChanged();
};

#endif

