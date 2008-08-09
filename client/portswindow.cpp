#include "portswindow.h"
#include "streamconfigdialog.h"
#include <QInputDialog>
#include <QItemSelectionModel>

PortsWindow::PortsWindow(PortGroupList *pgl, QWidget *parent)
{
	//slm = new StreamListModel();
	//plm = new PortGroupList();
	plm = pgl;

	setupUi(this);

	tvPortList->addAction(actionNew_Port_Group);
	tvPortList->addAction(actionDelete_Port_Group);
	tvPortList->addAction(actionConnect_Port_Group);
	tvPortList->addAction(actionDisconnect_Port_Group);

	tvStreamList->addAction(actionNew_Stream);
	tvStreamList->addAction(actionDelete_Stream);

	tvStreamList->setModel(plm->getStreamModel());
	tvPortList->setModel(plm->getPortModel());
	
	connect( plm->getPortModel(), 
		SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), 
		this, SLOT(when_portModel_dataChanged(const QModelIndex&, 
			const QModelIndex&)));
	connect( tvPortList->selectionModel(), 
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
		this, SLOT(when_portView_currentChanged(const QModelIndex&, 
			const QModelIndex&)));
	connect( tvStreamList->selectionModel(), 
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
		this, SLOT(when_streamView_currentChanged(const QModelIndex&, 
			const QModelIndex&)));
	connect( tvStreamList->selectionModel(), 
		SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this, SLOT(when_streamView_selectionChanged()));
	connect( tvPortList->selectionModel(), 
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), 
		plm->getStreamModel(), SLOT(setCurrentPortIndex(const QModelIndex&)));
}

PortsWindow::~PortsWindow()
{
	delete plm;
}

void PortsWindow::on_tvStreamList_activated(const QModelIndex & index)
{
	StreamConfigDialog	*scd;

	if (!index.isValid())
	{
		qDebug("%s: invalid index", __FUNCTION__);
		return;
	}
	// FIXME(MED): This way of passing params must be changed
	scd = new StreamConfigDialog(plm->getStreamModel()->currentPortStreamList(),
		(uint) index.row(), this);
	qDebug("stream list activated\n");
	scd->exec(); // TODO: chk retval
	delete scd;
}

void PortsWindow::when_portView_currentChanged(const QModelIndex& current,
	const QModelIndex& previous)
{
	updatePortViewActions(current);

	if (!current.isValid())
	{	
		qDebug("setting stacked widget to blank page");
		swDetail->setCurrentIndex(2); // blank page
	}
	else
	{
		if (plm->isPortGroup(current))
		{
			swDetail->setCurrentIndex(1);	// portGroup detail page	
		}
		else if (plm->isPort(current))
		{
			swDetail->setCurrentIndex(0);	// port detail page	
		}
	}
}

void PortsWindow::when_streamView_currentChanged(const QModelIndex& current,
	const QModelIndex& previous)
{
	qDebug("stream view current changed");
	updateStreamViewActions();
}

void PortsWindow::when_streamView_selectionChanged()
{
	qDebug("stream view selection changed");
	updateStreamViewActions();
}

void PortsWindow::when_portModel_dataChanged(const QModelIndex& topLeft,
	const QModelIndex& bottomRight)
{
#if 0 // not sure why the >= <= operators are not overloaded in QModelIndex
	if ((tvPortList->currentIndex() >= topLeft) &&
		(tvPortList->currentIndex() <= bottomRight))
#endif
	if ((topLeft < tvPortList->currentIndex()) || 
			(topLeft == tvPortList->currentIndex()) &&
		((tvPortList->currentIndex() < bottomRight)) || 
			(tvPortList->currentIndex() == bottomRight))
	{
		updatePortViewActions(tvPortList->currentIndex());
	}
	
}

#if 0
void PortsWindow::updateStreamViewActions(const QModelIndex& current)
{
	if (current.isValid())
		actionDelete_Stream->setEnabled(true);
	else
		actionDelete_Stream->setDisabled(true);
}
#endif

void PortsWindow::updateStreamViewActions()
{
	if (tvStreamList->selectionModel()->hasSelection())
	{
		qDebug("Has selection %d",
			tvStreamList->selectionModel()->selection().size());
		// If more than one non-contiguous ranges selected, disable "New"
		if (tvStreamList->selectionModel()->selection().size() > 1)
			actionNew_Stream->setDisabled(true);
		else
			actionNew_Stream->setEnabled(true);

		// Delete is always enabled as long as we have a selection
		actionDelete_Stream->setEnabled(true);
	}
	else
	{
		qDebug("No selection");
		actionNew_Stream->setEnabled(true);
		actionDelete_Stream->setDisabled(true);
	}
}

void PortsWindow::updatePortViewActions(const QModelIndex& current)
{
	if (!current.isValid())
	{
		qDebug("current is now invalid");
		actionDelete_Port_Group->setDisabled(true);
		actionConnect_Port_Group->setDisabled(true);
		actionDisconnect_Port_Group->setDisabled(true);
	
		goto _EXIT;
	}

	qDebug("currentChanged %llx", current.internalId());

	if (plm->isPortGroup(current))
	{
		actionDelete_Port_Group->setEnabled(true);
		switch(plm->portGroup(current).state())
		{
			case QAbstractSocket::UnconnectedState:
			case QAbstractSocket::ClosingState:
				qDebug("state = unconnected|closing");
				actionConnect_Port_Group->setEnabled(true);
				actionDisconnect_Port_Group->setDisabled(true);
				break;

			case QAbstractSocket::HostLookupState:
			case QAbstractSocket::ConnectingState:
			case QAbstractSocket::ConnectedState:
				qDebug("state = lookup|connecting|connected");
				actionConnect_Port_Group->setDisabled(true);
				actionDisconnect_Port_Group->setEnabled(true);
				break;


			case QAbstractSocket::BoundState:
			case QAbstractSocket::ListeningState:
			default:
				// FIXME(LOW): indicate error
				qDebug("unexpected state");
				break;
		}
	}
	else if (plm->isPort(current))
	{
		actionDelete_Port_Group->setEnabled(false);
		actionConnect_Port_Group->setEnabled(false);
		actionDisconnect_Port_Group->setEnabled(false);
	}

_EXIT:
	return;
}

void PortsWindow::on_pbApply_clicked()
{
{
	// TODO (LOW): This block is for testing only
	QModelIndex	current = tvPortList->selectionModel()->currentIndex();

	if (current.isValid())
		qDebug("current =  %llx", current.internalId());
	else
		qDebug("current is invalid");
}
}

void PortsWindow::on_actionNew_Port_Group_triggered()
{
	bool ok;
	QString text = QInputDialog::getText(this, 
		"Add Port Group", "Port Group Address (IP[:Port])",
		QLineEdit::Normal, lastNewPortGroup, &ok);
	
	if (ok)
	{
		QStringList addr = text.split(":");
		if (addr.size() == 1) // Port unspecified
			addr.append(QString().setNum(DEFAULT_SERVER_PORT));
		PortGroup *pg = new PortGroup(QHostAddress(addr[0]),addr[1].toUShort());	
		plm->addPortGroup(*pg);
		lastNewPortGroup = text;
	}
}

void PortsWindow::on_actionDelete_Port_Group_triggered()
{
	QModelIndex	current = tvPortList->selectionModel()->currentIndex();

	if (current.isValid())
		plm->removePortGroup(plm->portGroup(current));
}

void PortsWindow::on_actionConnect_Port_Group_triggered()
{
	QModelIndex	current = tvPortList->selectionModel()->currentIndex();

	if (current.isValid())
		plm->portGroup(current).connectToHost();
}

void PortsWindow::on_actionDisconnect_Port_Group_triggered()
{
	QModelIndex	current = tvPortList->selectionModel()->currentIndex();

	if (current.isValid())
		plm->portGroup(current).disconnectFromHost();
}
#if 0
void PortsWindow::on_actionNew_Stream_triggered()
{
	qDebug("New Stream Action");

	int row = 0;

	if (tvStreamList->currentIndex().isValid())
		row = tvStreamList->currentIndex().row();
	plm->getStreamModel()->insertRows(row, 1);	
}

void PortsWindow::on_actionDelete_Stream_triggered()
{
	qDebug("Delete Stream Action");
	if (tvStreamList->currentIndex().isValid())
		plm->getStreamModel()->removeRows(tvStreamList->currentIndex().row(), 1);	
}
#endif

void PortsWindow::on_actionNew_Stream_triggered()
{
	qDebug("New Stream Action");

	// In case nothing is selected, insert 1 row at the top
	int row = 0, count = 1;

	// In case we have a single range selected; insert as many rows as
	// in the singe selected range before the top of the selected range
	if (tvStreamList->selectionModel()->selection().size() == 1)
	{
		row = tvStreamList->selectionModel()->selection().at(0).top();
		count = tvStreamList->selectionModel()->selection().at(0).height();
	}

	plm->getStreamModel()->insertRows(row, count);	
}

void PortsWindow::on_actionDelete_Stream_triggered()
{
	qDebug("Delete Stream Action");

	QModelIndex		index;

	if (tvStreamList->selectionModel()->hasSelection())
	{
		qDebug("SelectedIndexes %d",
			tvStreamList->selectionModel()->selectedRows().size());
		while(tvStreamList->selectionModel()->selectedRows().size())
		{
			index = tvStreamList->selectionModel()->selectedRows().at(0);
			plm->getStreamModel()->removeRows(index.row(), 1);	
		}
	}
	else
		qDebug("No selection");
}


