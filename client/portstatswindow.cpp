
#include "portstatswindow.h"
#include "portstatsmodel.h"
#include "portstatsfilterdialog.h"

#include "QHeaderView"

//PortStatsWindow::PortStatsWindow(QWidget *parent) : QDialog (parent)
PortStatsWindow::PortStatsWindow(PortGroupList *pgl, QWidget *parent)
{
	setupUi(this);

	model = pgl->getPortStatsModel();
	tvPortStats->setModel(model);
	tvPortStats->horizontalHeader()->setMovable(true);
}

PortStatsWindow::~PortStatsWindow()
{
}

void PortStatsWindow::on_tbFilter_clicked()
{
	bool ok;
	QList<uint>	currentColumns, newColumns;
	PortStatsFilterDialog	dialog;

	for(int i = 0; i < model->columnCount(); i++)
		if (!tvPortStats->isColumnHidden(i))
			currentColumns.append(i);

	newColumns = dialog.getItemList(&ok, model, Qt::Horizontal, currentColumns);

	if(ok)
		for(int i = 0; i < model->columnCount(); i++)
			tvPortStats->setColumnHidden(i, !newColumns.contains(i));
}
