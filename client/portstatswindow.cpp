
#include "portstatswindow.h"
#include "portstatsmodel.h"
#include "portstatsfilterdialog.h"

#include "QHeaderView"

PortStatsWindow::PortStatsWindow(PortGroupList *pgl, QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);

	this->pgl = pgl;
	model = pgl->getPortStatsModel();
	tvPortStats->setModel(model);
	tvPortStats->horizontalHeader()->setMovable(true);
	tvPortStats->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

PortStatsWindow::~PortStatsWindow()
{
}

/* ------------- SLOTS -------------- */

void PortStatsWindow::on_tbStartTransmit_clicked()
{
	// TODO(MED): get selected ports

	if (pgl->numPortGroups())
	{
		QList<uint>	portIdList;

		// FIXME(HI): Testing only!!!
		portIdList.append(1); // MS Loopback adapter
		pgl->portGroupByIndex(0).startTx(portIdList);
	}
}

void PortStatsWindow::on_tbStopTransmit_clicked()
{
	// TODO(MED)
}

void PortStatsWindow::on_tbStartCapture_clicked()
{
	// TODO(MED)
}

void PortStatsWindow::on_tbStopCapture_clicked()
{
	// TODO(MED)
}

void PortStatsWindow::on_tbViewCapture_clicked()
{
	// TODO(MED)
}

void PortStatsWindow::on_tbClear_clicked()
{
	// TODO(MED)
}

void PortStatsWindow::on_tbClearAll_clicked()
{
	for (int i = 0; i < pgl->numPortGroups(); i++)
	{
		pgl->portGroupByIndex(0).clearPortStats();
	}
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
