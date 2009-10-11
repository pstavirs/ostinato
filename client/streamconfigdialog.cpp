#include <QHostAddress>

#include "streamconfigdialog.h"
#include "stream.h"
#include "abstractprotocol.h"
#include "protocollistiterator.h"

#include "modeltest.h"

// FIXME(HI) - remove
#include "../common/protocolmanager.h"
extern ProtocolManager OstProtocolManager;

int StreamConfigDialog::lastTopLevelTabIndex = 0;

StreamConfigDialog::StreamConfigDialog(Port &port, uint streamIndex,
	QWidget *parent) : QDialog (parent), mPort(port)
{
	OstProto::Stream s;
	mCurrentStreamIndex = streamIndex;
	mpStream = new Stream;
	mPort.streamByIndex(mCurrentStreamIndex)->protoDataCopyInto(s);
	mpStream->protoDataCopyFrom(s);
	_iter = mpStream->createProtocolListIterator();
	isUpdateInProgress = false;

	setupUi(this);
	setupUiExtra();

	connect(bgL1Proto, SIGNAL(buttonClicked(int)), 
		this, SLOT(updateL1Protocol(int)));
	connect(bgL2Proto, SIGNAL(buttonClicked(int)), 
		this, SLOT(updateFrameTypeProtocol(int)));
	connect(bgVlan, SIGNAL(buttonClicked(int)),
		this, SLOT(updateVlanProtocol(int)));
	connect(bgL3Proto, SIGNAL(buttonClicked(int)),
		this, SLOT(updateL3Protocol(int)));
	connect(bgL4Proto, SIGNAL(buttonClicked(int)),
		this, SLOT(updateL4Protocol(int)));
	connect(bgPayloadProto, SIGNAL(buttonClicked(int)),
		this, SLOT(updatePayloadProtocol(int)));

	//! \todo causes a crash!
#if 0	
	connect(lePktLen, SIGNAL(textEdited(QString)),
		this, SLOT(updateContents()));
#endif

	// Time to play match the signals and slots!

	// Enable VLAN Choices only if FT = Eth2 or SNAP
#if 0
	connect(rbFtNone, SIGNAL(toggled(bool)), gbVlan, SLOT(setDisabled(bool)));
	connect(rbFtOther, SIGNAL(toggled(bool)), gbVlan, SLOT(setDisabled(bool)));
	connect(rbFtNone, SIGNAL(clicked(bool)), rbVlanNone, SLOT(click()));
#endif

	// Force all protocols = None if L1 = None
	connect(rbL1None, SIGNAL(clicked(bool)), rbVlanNone, SLOT(click()));
	connect(rbL1None, SIGNAL(clicked(bool)), rbFtNone, SLOT(click()));
	connect(rbL1None, SIGNAL(clicked(bool)), rbPayloadNone, SLOT(click()));

	connect(rbFtNone, SIGNAL(clicked(bool)), rbL3None, SLOT(click()));

	// Enable/Disable L3 Protocol Choices for FT None
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Enable/Disable L3 Protocol Choices for FT Ethernet2
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Force L3 = None if FT = 802.3 Raw
	connect(rbFt802Dot3Raw, SIGNAL(clicked(bool)), rbL3None, SLOT(click()));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Force L3 = None if FT = 802.3 LLC (to ensure a valid L3 is selected)
	connect(rbFt802Dot3Llc, SIGNAL(clicked(bool)), rbL3None, SLOT(click()));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Llc
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802.3 LLC SNAP
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Force L3 = Other if FT = Other
	connect(rbFtOther, SIGNAL(toggled(bool)), rbL3Other, SLOT(setChecked(bool)));
	connect(rbFtOther, SIGNAL(toggled(bool)), gbL3Proto, SLOT(setDisabled(bool)));

	// Enable/Disable L4 Protocol Choices for L3 Protocol None
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4None, SLOT(setEnabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Icmp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Igmp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Tcp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Udp, SLOT(setDisabled(bool)));

	// Force L4 Protocol = None if L3 Protocol is set to None
	connect(rbL3None, SIGNAL(clicked(bool)), rbL4None, SLOT(click()));

	// Enable/Disable L4 Protocol Choices for L3 Protocol IPv4
	connect(rbL3Ipv4, SIGNAL(toggled(bool)), rbL4None, SLOT(setEnabled(bool)));
	connect(rbL3Ipv4, SIGNAL(toggled(bool)), rbL4Icmp, SLOT(setEnabled(bool)));
	connect(rbL3Ipv4, SIGNAL(toggled(bool)), rbL4Igmp, SLOT(setEnabled(bool)));
	connect(rbL3Ipv4, SIGNAL(toggled(bool)), rbL4Tcp, SLOT(setEnabled(bool)));
	connect(rbL3Ipv4, SIGNAL(toggled(bool)), rbL4Udp, SLOT(setEnabled(bool)));

	// Enable/Disable L4 Protocol Choices for L3 Protocol ARP
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4None, SLOT(setEnabled(bool)));
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4Icmp, SLOT(setDisabled(bool)));
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4Igmp, SLOT(setDisabled(bool)));
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4Tcp, SLOT(setDisabled(bool)));
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4Udp, SLOT(setDisabled(bool)));

	// Force L4 Protocol = None if L3 Protocol is set to ARP
	connect(rbL3Arp, SIGNAL(clicked(bool)), rbL4None, SLOT(click()));

	// Force L4 = Other if L3 = Other
	connect(rbL3Other, SIGNAL(toggled(bool)), rbL4Other, SLOT(setChecked(bool)));
	connect(rbL3Other, SIGNAL(toggled(bool)), gbL4Proto, SLOT(setDisabled(bool)));

	// Force Payload = Other if L4 = Other
	connect(rbL4Other, SIGNAL(toggled(bool)), rbPayloadOther, SLOT(setChecked(bool)));
	connect(rbL4Other, SIGNAL(toggled(bool)), gbPayloadProto, SLOT(setDisabled(bool)));

	mpAvailableProtocolsModel = new QStringListModel(
		OstProtocolManager.protocolDatabase(), this);
	lvAllProtocols->setModel(mpAvailableProtocolsModel);
	mpSelectedProtocolsModel = new QStringListModel(this);
	lvSelectedProtocols->setModel(mpSelectedProtocolsModel);


	connect(lvAllProtocols->selectionModel(),
		SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this, SLOT(when_lvAllProtocols_selectionChanged(
			const QItemSelection&, const QItemSelection&)));
	connect(lvSelectedProtocols->selectionModel(),
		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
		this, SLOT(when_lvSelectedProtocols_currentChanged(const QModelIndex&,
			const QModelIndex&)));

	LoadCurrentStream();
	mpPacketModel = new PacketModel(this);
	tvPacketTree->setModel(mpPacketModel);
	mpPacketModelTester = new ModelTest(mpPacketModel);
	tvPacketTree->header()->hide();
	vwPacketDump->setModel(mpPacketModel);
	vwPacketDump->setSelectionModel(tvPacketTree->selectionModel());

	// TODO(MED):

	//! \todo Implement then enable these protocols - ARP, IPv6, ICMP, IGMP
	rbL3Arp->setHidden(true);
	rbL3Ipv6->setHidden(true);
	rbL4Icmp->setHidden(true);
	rbL4Igmp->setHidden(true);
	//! \todo Enable navigation of streams
	pbPrev->setDisabled(true);
	pbNext->setDisabled(true);
	//! \todo Support Goto Stream Id
	leStreamId->setDisabled(true);
	disconnect(rbActionGotoStream, SIGNAL(toggled(bool)), leStreamId, SLOT(setEnabled(bool)));
	//! \todo Support Continuous Mode
	rbModeContinuous->setDisabled(true);

	// Finally, restore the saved last selected tab for the various tab widgets
	twTopLevel->setCurrentIndex(lastTopLevelTabIndex);
}

void StreamConfigDialog::setupUiExtra()
{
	QRegExp reHex2B("[0-9,a-f,A-F]{1,4}");
	QRegExp reHex4B("[0-9,a-f,A-F]{1,8}");
	QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

	// ---- Setup default stuff that cannot be done in designer ----
#if 0
	gbVlan->setDisabled(true);
#endif

	bgL1Proto = new QButtonGroup();
	bgL1Proto->addButton(rbL1None, ButtonIdNone);
	bgL1Proto->addButton(rbL1Mac, OstProto::Protocol::kMacFieldNumber);
	bgL1Proto->addButton(rbL1Other, ButtonIdOther);

	bgL2Proto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbFrameType->findChildren<QRadioButton*>())
		bgL2Proto->addButton(btn);
#else
	bgL2Proto->addButton(rbFtNone, ButtonIdNone);
	bgL2Proto->addButton(rbFtEthernet2, OstProto::Protocol::kEth2FieldNumber);
	bgL2Proto->addButton(rbFt802Dot3Raw, OstProto::Protocol::kDot3FieldNumber);
	bgL2Proto->addButton(rbFt802Dot3Llc, OstProto::Protocol::kDot2LlcFieldNumber);
	bgL2Proto->addButton(rbFtLlcSnap, OstProto::Protocol::kDot2SnapFieldNumber);
	bgL2Proto->addButton(rbFtOther, ButtonIdOther);
#endif

	bgVlan = new QButtonGroup();
	bgVlan->addButton(rbVlanNone, ButtonIdNone);
	bgVlan->addButton(rbVlanSingle, OstProto::Protocol::kVlanFieldNumber);
	bgVlan->addButton(rbVlanDouble, OstProto::Protocol::kVlanStackFieldNumber);

	bgL3Proto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbL3Proto->findChildren<QRadioButton*>())
		bgL3Proto->addButton(btn);
#else
	bgL3Proto->addButton(rbL3None, ButtonIdNone);
	bgL3Proto->addButton(rbL3Ipv4, OstProto::Protocol::kIp4FieldNumber);
	bgL3Proto->addButton(rbL3Ipv6, 0xFFFF);
	bgL3Proto->addButton(rbL3Arp, 0xFFFF);
	bgL3Proto->addButton(rbL3Other, ButtonIdOther);
#endif

	bgL4Proto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbL4Proto->findChildren<QRadioButton*>())
		bgL4Proto->addButton(btn);
#else
	bgL4Proto->addButton(rbL4None, 0);
	bgL4Proto->addButton(rbL4Tcp, OstProto::Protocol::kTcpFieldNumber);
	bgL4Proto->addButton(rbL4Udp, OstProto::Protocol::kUdpFieldNumber);
	bgL4Proto->addButton(rbL4Icmp, 0xFFFF);
	bgL4Proto->addButton(rbL4Igmp, 0xFFFF);
	bgL4Proto->addButton(rbL4Other, ButtonIdOther);
#endif

	bgPayloadProto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbPayloadProto->findChildren<QRadioButton*>())
		bgPayloadProto->addButton(btn);
#else
	bgPayloadProto->addButton(rbPayloadNone, ButtonIdNone);
	bgPayloadProto->addButton(rbPayloadPattern, OstProto::Protocol::kPayloadFieldNumber);
	bgPayloadProto->addButton(rbPayloadOther, ButtonIdOther);
#endif
	/*
	** Setup Validators
	*/	
	// Meta Data
	//! \todo - doesn't seem to work -  range validator needs a spinbox?
	//lePktLen->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN, this));

	/*
	** Setup Connections
	*/
	connect(rbSendPackets, SIGNAL(toggled(bool)), 
		this, SLOT(update_NumPacketsAndNumBursts()));
	connect(rbSendBursts, SIGNAL(toggled(bool)), 
		this, SLOT(update_NumPacketsAndNumBursts()));
	connect(rbModeFixed, SIGNAL(toggled(bool)), 
		this, SLOT(update_NumPacketsAndNumBursts()));
	connect(rbModeContinuous, SIGNAL(toggled(bool)), 
		this, SLOT(update_NumPacketsAndNumBursts()));

}

StreamConfigDialog::~StreamConfigDialog()
{
	delete mpPacketModelTester;
	delete mpPacketModel;

	delete bgL1Proto;
	delete bgL2Proto;
	delete bgVlan;
	delete bgL3Proto;
	delete bgL4Proto;
	delete bgPayloadProto;

	delete _iter;
	delete mpStream;
}

void StreamConfigDialog::on_cmbPktLenMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
	{
		lePktLen->setEnabled(true);
		lePktLenMin->setDisabled(true);
		lePktLenMax->setDisabled(true);
	}
	else if (mode == "Increment")
	{
		lePktLen->setDisabled(true);
		lePktLenMin->setEnabled(true);
		lePktLenMax->setEnabled(true);
	}
	else if (mode == "Decrement")
	{
		lePktLen->setDisabled(true);
		lePktLenMin->setEnabled(true);
		lePktLenMax->setEnabled(true);
	}
	else if (mode == "Random")
	{
		lePktLen->setDisabled(true);
		lePktLenMin->setEnabled(true);
		lePktLenMax->setEnabled(true);
	}
	else
	{
		qWarning("Unhandled/Unknown PktLenMode = %s", mode.toAscii().data());
	}
}

void StreamConfigDialog::on_pbPrev_clicked()
{
#if 0
	StoreCurrentStream(currStreamIdx);
	currStreamIdx--;
	LoadCurrentStream(currStreamIdx);

	pbPrev->setDisabled((currStreamIdx == 0));
	pbNext->setDisabled((currStreamIdx == 2));
#endif
}

void StreamConfigDialog::on_pbNext_clicked()
{
#if 0
	StoreCurrentStream(currStreamIdx);
	currStreamIdx++;
	LoadCurrentStream(currStreamIdx);

	pbPrev->setDisabled((currStreamIdx == 0));
	pbNext->setDisabled((currStreamIdx == 2));
#endif
}

void StreamConfigDialog::on_tbSelectProtocols_currentChanged(int index)
{
	qDebug("%s, index = %d", __FUNCTION__, index);
	switch (index)
	{
		case 0:
			updateSelectProtocolsSimpleWidget();
			break;
		case 1:
			updateSelectProtocolsAdvancedWidget();
			break;
		default:
			qFatal("%s: unexpected index = %d", __FUNCTION__, index);
	}
}

void StreamConfigDialog::when_lvAllProtocols_selectionChanged(
	const QItemSelection &selected, const QItemSelection &deselected)
{
	int size = lvAllProtocols->selectionModel()->selectedIndexes().size();

	qDebug("%s: selected.indexes().size = %d\n", __FUNCTION__, size);

	tbAdd->setEnabled(size > 0);
}

void StreamConfigDialog::when_lvSelectedProtocols_currentChanged(
	const QModelIndex &current, const QModelIndex &previous)
{
	qDebug("%s: currentRow = %d\n", __FUNCTION__, current.row());

	tbDelete->setEnabled(current.isValid());
	tbUp->setEnabled(current.isValid() && (current.row() != 0));
	tbDown->setEnabled(current.isValid() && 
		(current.row() != (current.model()->rowCount() - 1)));
}

void StreamConfigDialog::on_tbAdd_clicked()
{
	int n = 0;
	QModelIndex idx2;
	AbstractProtocol *p;
	QModelIndexList	selection;

	selection = lvAllProtocols->selectionModel()->selectedIndexes();

	// Validation
	if (selection.size() == 0)
		return;

	idx2 = lvSelectedProtocols->currentIndex();
	if (idx2.isValid())
		n = idx2.row();

	_iter->toFront();
	while (n--)
	{
		if (!_iter->hasNext())
			return;

		p = _iter->next();
	}

	foreach(QModelIndex idx, selection)
		_iter->insert(OstProtocolManager.createProtocol(
			mpAvailableProtocolsModel->stringList().at(idx.row()), mpStream));

	updateSelectProtocolsAdvancedWidget();
	lvSelectedProtocols->setCurrentIndex(idx2);
}

void StreamConfigDialog::on_tbDelete_clicked()
{
	int n;
	QModelIndex idx;
	AbstractProtocol *p;

	idx = lvSelectedProtocols->currentIndex();

	// Validation
	if (!idx.isValid())
		return;

	n = idx.row() + 1;

	_iter->toFront();
	while (n--)
	{
		if (!_iter->hasNext())
			return;

		p = _iter->next();
	}

	_iter->remove();
	delete p;

	updateSelectProtocolsAdvancedWidget();
	lvSelectedProtocols->setCurrentIndex(idx);
}

void StreamConfigDialog::on_tbUp_clicked()
{
	int m, n;
	QModelIndex idx;
	AbstractProtocol *p;

	idx = lvSelectedProtocols->currentIndex();

	// Validation
	if (!idx.isValid() || idx.row() == 0)
		return;

	m = n = idx.row() + 1;

	_iter->toFront();
	while (n--)
	{
		if (!_iter->hasNext())
			return;

		p = _iter->next();
	}

	_iter->remove();
	_iter->previous();
	_iter->insert(p);

	updateSelectProtocolsAdvancedWidget();
	lvSelectedProtocols->setCurrentIndex(idx.sibling(m-2, 0));
}

void StreamConfigDialog::on_tbDown_clicked()
{
	int m, n;
	QModelIndex idx;
	AbstractProtocol *p;

	idx = lvSelectedProtocols->currentIndex();

	// Validation
	if (!idx.isValid() || idx.row() == idx.model()->rowCount())
		return;

	m = n = idx.row() + 1;

	_iter->toFront();
	while (n--)
	{
		if (!_iter->hasNext())
			return;

		p = _iter->next();
	}

	_iter->remove();
	_iter->next();
	_iter->insert(p);

	updateSelectProtocolsAdvancedWidget();
	lvSelectedProtocols->setCurrentIndex(idx.sibling(m,0));
}

void StreamConfigDialog::updateSelectProtocolsAdvancedWidget()
{
	QStringList	selProtoList;

	qDebug("%s", __FUNCTION__);

	_iter->toFront();
	while(_iter->hasNext())
	{
		AbstractProtocol* p = _iter->next();
		qDebug("%p -- %d", p, p->protocolNumber());
		selProtoList.append(p->shortName());
	}
	mpSelectedProtocolsModel->setStringList(selProtoList);
}

void StreamConfigDialog::on_twTopLevel_currentChanged(int index)
{
	switch (index)
	{
		// Protocol Data
		case 1:
		{
			QWidget *selWidget;

			// Hide the ToolBox before modifying it - else we have a crash !!!
			tbProtocolData->hide();

			selWidget = tbProtocolData->currentWidget();

			// Remove all existing protocol widgets 
			while (tbProtocolData->count() > 0)
			{
				QWidget* w = tbProtocolData->widget(0);
				tbProtocolData->removeItem(0);
				w->setParent(0);
			}

			// Repopulate the widgets
			_iter->toFront();
			while (_iter->hasNext())
			{
				AbstractProtocol* p = _iter->next();
				tbProtocolData->addItem(p->configWidget(), p->name());
			}

			tbProtocolData->setCurrentWidget(selWidget);

			tbProtocolData->show();
			break;
		}

		// Packet View
		case 3:
		{
			StoreCurrentStream();
			mpPacketModel->setSelectedProtocols(*_iter);
			break;
		}

		default:
			break;
	}
}

void StreamConfigDialog::update_NumPacketsAndNumBursts()
{
	if (rbSendPackets->isChecked() && rbModeFixed->isChecked())
		leNumPackets->setEnabled(true);
	else
		leNumPackets->setEnabled(false);

	if (rbSendBursts->isChecked() && rbModeFixed->isChecked())
		leNumBursts->setEnabled(true);
	else
		leNumBursts->setEnabled(false);
}

#if 0
void StreamConfigDialog::on_lePattern_editingFinished()
{
	ulong	num = 0;
	bool	isOk;
	QString	str;

	num = lePattern->text().remove(QChar(' ')).toULong(&isOk, 16);
	qDebug("editfinished (%s | %x)\n", lePattern->text().toAscii().data(), num);
	lePattern->setText(uintToHexStr(num, str, 4));
	qDebug("editfinished (%s | %x)\n", lePattern->text().toAscii().data(), num);
}
#endif

/*! 
Skip protocols upto and including the layer specified.
	0 - L1
	1 - VLAN
	2 - L2
	3 - L3
	4 - L4
TODO: Convert the above values to enum??
*/
bool StreamConfigDialog::skipProtocols(int layer)
{
	int id;
	QAbstractButton *btn;

	_iter->toFront();

	// Skip L1
	if (_iter->hasNext())
	{
		id = _iter->next()->protocolNumber();
		btn = bgL1Proto->button(id);
		if (btn == NULL)
			_iter->previous();
	}

	if (layer == 0)
		goto _done;

	// Skip VLAN
	if(_iter->hasNext())
	{
		id = _iter->next()->protocolNumber();
		btn = bgVlan->button(id);
		if (btn == NULL)
			_iter->previous();
	}

	if (layer == 1)
		goto _done;

	// Skip L2
	if(_iter->hasNext())
	{
		id = _iter->next()->protocolNumber();
		btn = bgL2Proto->button(id);
		if (btn == NULL)
			_iter->previous();
	}

	if (layer == 2)
		goto _done;

	// Skip L3
	if (_iter->hasNext())
	{
		id = _iter->next()->protocolNumber();
		btn = bgL3Proto->button(id);
		if (btn == NULL)
			_iter->previous();
	}

	if (layer == 3)
		goto _done;

	// Skip L4
	if(_iter->hasNext())
	{
		id = _iter->next()->protocolNumber();
		btn = bgL4Proto->button(id);
		if (btn == NULL)
			_iter->previous();
	}

	if (layer == 4)
		goto _done;

	return false;

_done:
	return true;
}

void StreamConfigDialog::updateL1Protocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		AbstractProtocol *p;

		_iter->toFront();

		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updateVlanProtocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		int ret;
		AbstractProtocol *p;

		ret = skipProtocols(0);

		Q_ASSERT(ret == true);
		Q_ASSERT(oldId != ButtonIdOther);
		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updateFrameTypeProtocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		int ret;
		AbstractProtocol *p;

		ret = skipProtocols(1);

		Q_ASSERT(ret == true);
		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updateL3Protocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		int ret;
		AbstractProtocol *p;

		ret = skipProtocols(2);

		Q_ASSERT(ret == true);
		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updateL4Protocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		int ret;
		AbstractProtocol *p;

		ret = skipProtocols(3);

		Q_ASSERT(ret == true);
		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updatePayloadProtocol(int newId)
{
	static int oldId;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		int ret;
		AbstractProtocol *p;

		ret = skipProtocols(4);

		Q_ASSERT(ret == true);
		Q_ASSERT(newId != ButtonIdOther);
		
		switch (oldId)
		{
			case ButtonIdNone:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case ButtonIdOther:
			default:
				Q_ASSERT(_iter->hasNext());
				p =_iter->next();

				if (newId)
					_iter->setValue(OstProtocolManager.createProtocol(
							newId, mpStream));
				else
					_iter->remove();
				delete p;
				while (_iter->hasNext())
				{

					p = _iter->next();
					_iter->remove();
					delete p;
				}
				break;
		}
	}

	oldId = newId;
	return;
}

void StreamConfigDialog::updateSelectProtocolsSimpleWidget()
{
	quint32			id;
	QAbstractButton *btn;

	qDebug("%s", __FUNCTION__);

	isUpdateInProgress = true;

	// Reset to default state
	rbL1None->setChecked(true);
	rbVlanNone->setChecked(true);
	rbFtNone->setChecked(true);
	rbL3None->setChecked(true);
	rbL4None->setChecked(true);
	rbPayloadNone->setChecked(true);

	_iter->toFront();

	// L1 (optional if followed by Payload)
	if (!_iter->hasNext()) // No protocols at all?
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgL1Proto->button(id);

	if (btn && btn->isEnabled())
		btn->click();
	else
	{
		btn = bgPayloadProto->button(id);
		if (btn && btn->isEnabled())
			goto _payload;
		else
			goto _otherL1;
	}

	// VLAN (optional)
	if (!_iter->hasNext())
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgVlan->button(id);

	if (btn && btn->isEnabled())
		btn->click();
	else
		_iter->previous();

	// L2 (optional if followed by Payload)
	if (!_iter->hasNext())
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgL2Proto->button(id);

	if (btn && btn->isEnabled())
		btn->click();
	else
	{
		btn = bgPayloadProto->button(id);
		if (btn && btn->isEnabled())
			goto _payload;
		else
			goto _otherL2;
	}

	// L3 (optional if followed by Payload)
	if (!_iter->hasNext())
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgL3Proto->button(id);

	if (btn && btn->isEnabled())
		btn->click();
	else
	{
		btn = bgPayloadProto->button(id);
		if (btn && btn->isEnabled())
			goto _payload;
		else
			goto _otherL3;
	}

	// L4 (optional if followed by Payload)
	if (!_iter->hasNext())
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgL4Proto->button(id);

	if (btn && btn->isEnabled())
		btn->click();
	else
	{
		btn = bgPayloadProto->button(id);
		if (btn && btn->isEnabled())
			goto _payload;
		else
			goto _otherL4;
	}

	// Payload Data
	if (!_iter->hasNext())
		goto _done;

	id = _iter->next()->protocolNumber();
	btn = bgPayloadProto->button(id);

_payload:
	if (btn && btn->isEnabled())
		btn->click();
	else
		goto _otherPayload;

	// If more protocol(s) beyond payload ...
	if (_iter->hasNext())
		goto _otherPayload;

	goto _done;

_otherL1:
	bgL1Proto->button(ButtonIdOther)->setChecked(true);
	updateL1Protocol(ButtonIdOther);
_otherL2:
	bgL2Proto->button(ButtonIdOther)->setChecked(true);
	updateFrameTypeProtocol(ButtonIdOther);
_otherL3:
	bgL3Proto->button(ButtonIdOther)->setChecked(true);
	updateL3Protocol(ButtonIdOther);
_otherL4:
	bgL4Proto->button(ButtonIdOther)->setChecked(true);
	updateL4Protocol(ButtonIdOther);
_otherPayload:
	bgPayloadProto->button(ButtonIdOther)->setChecked(true);
	updatePayloadProtocol(ButtonIdOther);

_done:
	isUpdateInProgress = false;

	return;
}

void StreamConfigDialog::LoadCurrentStream()
{
	QString	str;

	qDebug("loading mpStream %p", mpStream);

	// Meta Data
	{
		cmbPktLenMode->setCurrentIndex(mpStream->lenMode());
		lePktLen->setText(str.setNum(mpStream->frameLen()));
		lePktLenMin->setText(str.setNum(mpStream->frameLenMin()));
		lePktLenMax->setText(str.setNum(mpStream->frameLenMax()));
	}

	// Protocols
	{
		updateSelectProtocolsSimpleWidget();
		updateSelectProtocolsAdvancedWidget();

		mpStream->loadProtocolWidgets();
	}

	// Stream Control
	{
		switch (mpStream->sendUnit())
		{
		case Stream::e_su_packets:
			rbSendPackets->setChecked(true);
			break;
		case Stream::e_su_bursts:
			rbSendBursts->setChecked(true);
			break;
		default:
			qWarning("Unhandled sendUnit = %d\n", mpStream->sendUnit());
		}

		switch (mpStream->sendMode())
		{
		case Stream::e_sm_fixed:
			rbModeFixed->setChecked(true);
			break;
		case Stream::e_sm_continuous:
			rbModeContinuous->setChecked(true);
			break;
		default:
			qWarning("Unhandled sendMode = %d\n", mpStream->sendMode());
		}

		switch(mpStream->nextWhat())
		{
		case Stream::e_nw_stop:
			rbActionStop->setChecked(true);
			break;
		case Stream::e_nw_goto_next:
			rbActionGotoNext->setChecked(true);
			break;
		case Stream::e_nw_goto_id:
			rbActionGotoStream->setChecked(true);
			break;
		default:
			qWarning("Unhandled nextAction = %d\n", mpStream->nextWhat());
		}

		leNumPackets->setText(QString().setNum(mpStream->numPackets()));
		leNumBursts->setText(QString().setNum(mpStream->numBursts()));
		lePacketsPerBurst->setText(QString().setNum(mpStream->burstSize()));
		lePacketsPerSec->setText(QString().setNum(mpStream->packetRate()));
		leBurstsPerSec->setText(QString().setNum(mpStream->burstRate()));
		// TODO(MED): Change this when we support goto to specific stream
		leStreamId->setText(QString("0"));
	}
	qDebug("loading stream done");
}

void StreamConfigDialog::StoreCurrentStream()
{
	QString	str;
	bool	isOk;
	Stream	*pStream = mpStream;

	qDebug("storing pStream %p", pStream);

	// Meta Data
	pStream->setLenMode((Stream::FrameLengthMode) cmbPktLenMode->currentIndex());
	pStream->setFrameLen(lePktLen->text().toULong(&isOk));
	pStream->setFrameLenMin(lePktLenMin->text().toULong(&isOk));
	pStream->setFrameLenMax(lePktLenMax->text().toULong(&isOk));

	// Protocols
	{
		pStream->storeProtocolWidgets();
	}

	// Stream Control
	{
		if (rbSendPackets->isChecked())
			pStream->setSendUnit(Stream::e_su_packets);
		if (rbSendBursts->isChecked())
			pStream->setSendUnit(Stream::e_su_bursts);

		if (rbModeFixed->isChecked())
			pStream->setSendMode(Stream::e_sm_fixed);
		if (rbModeContinuous->isChecked())
			pStream->setSendMode(Stream::e_sm_continuous);

		if (rbActionStop->isChecked())
			pStream->setNextWhat(Stream::e_nw_stop);
		if (rbActionGotoNext->isChecked())
			pStream->setNextWhat(Stream::e_nw_goto_next);
		if (rbActionGotoStream->isChecked())
			pStream->setNextWhat(Stream::e_nw_goto_id);

		pStream->setNumPackets(leNumPackets->text().toULong(&isOk));
		pStream->setNumBursts(leNumBursts->text().toULong(&isOk));
		pStream->setBurstSize(lePacketsPerBurst->text().toULong(&isOk));
		pStream->setPacketRate(lePacketsPerSec->text().toULong(&isOk));
		pStream->setBurstRate(leBurstsPerSec->text().toULong(&isOk));
	}
}

void StreamConfigDialog::on_pbOk_clicked()
{
	OstProto::Stream	s;

	// Store dialog contents into stream
	StoreCurrentStream();

	// Copy the data from the "local working copy of stream" to "actual stream"
	mpStream->protoDataCopyInto(s);
	mPort.streamByIndex(mCurrentStreamIndex)->protoDataCopyFrom(s);

	qDebug("stream stored");

	lastTopLevelTabIndex = twTopLevel->currentIndex();
}

