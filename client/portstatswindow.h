#ifndef _PORT_STATS_WINDOW_H
#define _PORT_STATS_WINDOW_H

#include <QDialog>
#include <QAbstractItemModel>
#include "ui_portstatswindow.h"
#include "portgrouplist.h"

class PortStatsWindow : public QWidget, public Ui::PortStatsWindow
{
	Q_OBJECT

public:
	PortStatsWindow(PortGroupList *pgl, QWidget *parent = 0);
	~PortStatsWindow();

private:
	PortGroupList		*pgl;
	QAbstractItemModel	*model;

private slots:
	void on_tbStartTransmit_clicked();
	void on_tbStopTransmit_clicked();

	void on_tbStartCapture_clicked();
	void on_tbStopCapture_clicked();
	void on_tbViewCapture_clicked();

	void on_tbClear_clicked();
	void on_tbClearAll_clicked();

	void on_tbFilter_clicked();
};

#endif

