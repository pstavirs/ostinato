#ifndef _PORT_STATS_WINDOW_H
#define _PORT_STATS_WINDOW_H

#include <QDialog>
#include <QAbstractItemModel>
#include "ui_portstatswindow.h"
#include "portgrouplist.h"

class PortStatsWindow : public QDialog, public Ui::PortStatsWindow
{
	Q_OBJECT

public:
	PortStatsWindow(PortGroupList *pgl, QWidget *parent = 0);
	~PortStatsWindow();
private:
	QAbstractItemModel	*model;

private slots:
	void on_tbFilter_clicked();
};

#endif

