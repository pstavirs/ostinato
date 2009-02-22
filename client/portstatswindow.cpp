
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

	tvPortStats->verticalHeader()->setHighlightSections(false);
	tvPortStats->verticalHeader()->setDefaultSectionSize(
		tvPortStats->verticalHeader()->minimumSectionSize());

}

PortStatsWindow::~PortStatsWindow()
{
}

/* ------------- SLOTS -------------- */

void PortStatsWindow::on_tbStartTransmit_clicked()
{
	QList<PortStatsModel::PortGroupAndPortList>	pgpl;

	// Get selected ports
	model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
		   	pgpl);

	// Clear selected ports, portgroup by portgroup
	for (int i = 0; i < pgpl.size(); i++)
	{
		pgl->portGroupByIndex(pgpl.at(i).portGroupId).
			startTx(&pgpl[i].portList);
	}
}

void PortStatsWindow::on_tbStopTransmit_clicked()
{
	QList<PortStatsModel::PortGroupAndPortList>	pgpl;

	// Get selected ports
	model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
		   	pgpl);

	// Clear selected ports, portgroup by portgroup
	for (int i = 0; i < pgpl.size(); i++)
	{
		pgl->portGroupByIndex(pgpl.at(i).portGroupId).
			stopTx(&pgpl[i].portList);
	}
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
	QList<PortStatsModel::PortGroupAndPortList>	portList;

	// Get selected ports
	model->portListFromIndex(tvPortStats->selectionModel()->selectedColumns(),
		   	portList);

	// Clear selected ports, portgroup by portgroup
	for (int i = 0; i < portList.size(); i++)
	{
		pgl->portGroupByIndex(portList.at(i).portGroupId).
			clearPortStats(&portList[i].portList);
	}
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

	if (ok)
	{
		// hide/show sections first ...
		for(int i = 0; i < model->columnCount(); i++)
			tvPortStats->setColumnHidden(i, !newColumns.contains(i));

		// ... then for the 'shown' columns, set the visual index
		for(int i = 0; i < newColumns.size(); i++)
		{
			tvPortStats->horizontalHeader()->moveSection(tvPortStats->
					horizontalHeader()->visualIndex(newColumns.at(i)), i);
		}
	}
}
