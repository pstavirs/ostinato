#include "portstatswindow.h"
#include "portstatsmodel.h"

//PortStatsWindow::PortStatsWindow(QWidget *parent) : QDialog (parent)
PortStatsWindow::PortStatsWindow(PortGroupList *pgl, QWidget *parent)
{
	setupUi(this);

	tvPortStats->setModel(pgl->getPortStatsModel());

}

PortStatsWindow::~PortStatsWindow()
{
}
