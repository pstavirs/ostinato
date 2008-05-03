#include "portstatsfilterdialog.h"

PortStatsFilterDialog::PortStatsFilterDialog(AbstractItemModel *allPortsModel,
	QWidget *parent)
{
	setupUi(this);

	lvAllPorts->setModel(allPortsModel);
}
