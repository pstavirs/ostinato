#ifndef _PORTS_WINDOW_H
#define _PORTS_WINDOW_H

#include <QWidget>
#include <QAbstractItemModel>
#include "ui_portswindow.h"
#include "portgrouplist.h"

/* TODO
HIGH
MED
LOW
*/


class PortsWindow : public QWidget, private Ui::PortsWindow
{
	Q_OBJECT

	//QAbstractItemModel	*slm; // stream list model
	PortGroupList		*plm;

public:
	PortsWindow(PortGroupList *pgl, QWidget *parent = 0);
	~PortsWindow();

private:
	QString		lastNewPortGroup;

	void updatePortViewActions(const QModelIndex& current);
	//void updateStreamViewActions(const QModelIndex& current);
	void updateStreamViewActions();

private slots:
	void on_tvStreamList_activated(const QModelIndex & index);
	void when_portView_currentChanged(const QModelIndex& current,
		const QModelIndex& previous);
	void when_streamView_currentChanged(const QModelIndex& current,
		const QModelIndex& previous);
	void when_streamView_selectionChanged();
	void when_portModel_dataChanged(const QModelIndex& topLeft,
		const QModelIndex& bottomRight);

	void on_pbApply_clicked();	

	void on_actionNew_Port_Group_triggered();
	void on_actionDelete_Port_Group_triggered();
	void on_actionConnect_Port_Group_triggered();
	void on_actionDisconnect_Port_Group_triggered();

	void on_actionNew_Stream_triggered();
	void on_actionEdit_Stream_triggered();
	void on_actionDelete_Stream_triggered();
};

#endif

