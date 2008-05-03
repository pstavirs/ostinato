#ifndef _PORT_STATS_FILTER_DIALOG_H
#define _PORT_STATS_FILTER_DIALOG_H

#include <QDialog>
#include <QAbstractItemModel>
#include "ui_portstatsfilterdialog.h"
#include "portgrouplist.h"

class PortStatsFilterDialog : public QDialog, public Ui::PortStatsFilterDialog
{
	Q_OBJECT

public:
	PortStatsFilterDialog(AbstractItemModel *allPortsModel, 
		QWidget *parent = 0);
};

#endif

