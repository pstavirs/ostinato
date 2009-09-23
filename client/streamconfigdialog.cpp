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

	connect(bgFrameType, SIGNAL(buttonClicked(int)), 
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
	connect(rbFtNone, SIGNAL(toggled(bool)), gbVlan, SLOT(setDisabled(bool)));
	connect(rbFtOther, SIGNAL(toggled(bool)), gbVlan, SLOT(setDisabled(bool)));
	connect(rbFtNone, SIGNAL(clicked(bool)), rbVlanNone, SLOT(click()));
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
	gbVlan->setDisabled(true);

	bgFrameType = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbFrameType->findChildren<QRadioButton*>())
		bgFrameType->addButton(btn);
#else
	bgFrameType->addButton(rbFtNone, 0);
	bgFrameType->addButton(rbFtEthernet2, 121);
	bgFrameType->addButton(rbFt802Dot3Raw, 122);
	bgFrameType->addButton(rbFt802Dot3Llc, 123);
	bgFrameType->addButton(rbFtLlcSnap, 124);
	bgFrameType->addButton(rbFtOther, -1);
#endif

	bgVlan = new QButtonGroup();
	bgVlan->addButton(rbVlanNone, 0);
	bgVlan->addButton(rbVlanSingle, 126);
	bgVlan->addButton(rbVlanDouble, 127);

	bgL3Proto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbL3Proto->findChildren<QRadioButton*>())
		bgL3Proto->addButton(btn);
#else
	bgL3Proto->addButton(rbL3None, 0);
	bgL3Proto->addButton(rbL3Ipv4, 130);
	bgL3Proto->addButton(rbL3Ipv6, -1);
	bgL3Proto->addButton(rbL3Arp, -1);
	bgL3Proto->addButton(rbL3Other, -1);
#endif

	bgL4Proto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbL4Proto->findChildren<QRadioButton*>())
		bgL4Proto->addButton(btn);
#else
	bgL4Proto->addButton(rbL4None, 0);
	bgL4Proto->addButton(rbL4Tcp, 140);
	bgL4Proto->addButton(rbL4Udp, 141);
	bgL4Proto->addButton(rbL4Icmp, -1);
	bgL4Proto->addButton(rbL4Igmp, -1);
	bgL4Proto->addButton(rbL4Other, -1);
#endif

	bgPayloadProto = new QButtonGroup();
#if 0
	foreach(QRadioButton *btn, gbPayloadProto->findChildren<QRadioButton*>())
		bgPayloadProto->addButton(btn);
#else
	bgPayloadProto->addButton(rbPayloadPattern, 52);
	bgPayloadProto->addButton(rbPayloadOther, -1);
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

	delete bgFrameType;
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

bool StreamConfigDialog::skipProtocols(int layer)
{
#define CHK(p) (_iter->hasNext() && _iter->peekNext()->protocolNumber() == p)

	_iter->toFront();

	// Check and skip 'mac'
	if (CHK(51))
		_iter->next();
	else
		goto _unexpected_proto;

	if (layer == 0)
		goto _done;

	// Skip VLANs
	while (CHK(126))
		_iter->next();

	if (layer == 1)
		goto _done;

	// Skip L2 (FrameType)
	if (CHK(121)) // Eth2
		_iter->next();
	else if (CHK(122)) // 802.3 RAW
	{
		_iter->next();
		if (CHK(123)) // 802.3 LLC
		{
			_iter->next();
			if (CHK(124)) // SNAP
				_iter->next();
		}
	}
	else
		goto _unexpected_proto;

	if (layer == 2)
		goto _done;

	// Skip L3
	if (CHK(130)) // IP4
		_iter->next();
	else if (CHK(131)) // ARP
		_iter->next();
	else
		goto _unexpected_proto;

	if (layer == 3)
		goto _done;

	// Skip L4
	if (CHK(140)) // TCP
		_iter->next();
	else if (CHK(141)) // UDP
		_iter->next();
	else if (CHK(142)) // ICMP
		_iter->next();
	else if (CHK(143)) // IGMP
		_iter->next();
	else
		goto _unexpected_proto;

	if (layer == 4)
		goto _done;

	goto _unexpected_proto;

_done:
	return true;

_unexpected_proto:
	qWarning("%s: unexpected protocol", __FUNCTION__);
	return false;

#undef CHK
}

void StreamConfigDialog::updateFrameTypeProtocol(int newId)
{
	static int oldId = 0;
	AbstractProtocol *p;

	qDebug("%s:old id = %d new id = %d, upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		Q_ASSERT(newId != -1);

		if (!skipProtocols(1))
			goto _error;
		
		// Delete old Id
		switch (oldId)
		{
			case 0:
				break;
			case -1:
				// Blindly remove the current protocol
				p =_iter->next();
				_iter->remove();
				delete p;
				break;

			case 121: // ethernet
			case 122: // dot3
				p =_iter->next();
				if (p->protocolNumber() != (quint32) oldId)
					goto _error;

				_iter->remove();
				delete p;
				break;
			case 123: // dot3 llc
				p =_iter->next();
				if (p->protocolNumber() != 122)
					goto _error;
				_iter->remove();
				delete p;

				p =_iter->next();
				if (p->protocolNumber() != 123)
					goto _error;
				_iter->remove();
				delete p;
				break;
			case 124: // dot3 llc snap
				p =_iter->next();
				if (p->protocolNumber() != 122)
					goto _error;
				_iter->remove();
				delete p;

				p =_iter->next();
				if (p->protocolNumber() != 123)
					goto _error;
				_iter->remove();
				delete p;

				p =_iter->next();
				if (p->protocolNumber() != 124)
					goto _error;
				_iter->remove();
				delete p;
				break;
			default:
				goto _error;
		}

		// Insert new Id
		switch (newId)
		{
			case 0:
				break;
			case 121: // ethernet
				_iter->insert(OstProtocolManager.createProtocol(
					newId, mpStream));
				break;
			case 122: // dot3
				_iter->insert(OstProtocolManager.createProtocol(
					newId, mpStream));
				break;
			case 123: // dot3 llc
				_iter->insert(OstProtocolManager.createProtocol(
					122, mpStream));
				_iter->insert(OstProtocolManager.createProtocol(
					newId, mpStream));
				break;
			case 124: // dot3 llc snap
				_iter->insert(OstProtocolManager.createProtocol(
					122, mpStream));
				_iter->insert(OstProtocolManager.createProtocol(
					123, mpStream));
				_iter->insert(OstProtocolManager.createProtocol(
					newId, mpStream));
				break;
			default:
				goto _error;
		}
	}

	oldId = newId;
	return;

_error:
	qFatal("%s: unexpected incident", __FUNCTION__);
}

void StreamConfigDialog::updateVlanProtocol(int newId)
{
	static int oldId = 0;
	AbstractProtocol *p;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		Q_ASSERT(newId != -1);

		if (!skipProtocols(0))
			goto _error;
		
		// Delete oldId proto
		switch (oldId)
		{
			case 0:
				switch (newId)
				{
					case 127:
						_iter->insert(OstProtocolManager.createProtocol(
								126, mpStream));
						// No 'break'; fallthrough - by design!
					case 126:
						_iter->insert(OstProtocolManager.createProtocol(
								126, mpStream));
						break;
					default:
						goto _error;
				}
				break;

			case 126:
				if (!_iter->hasNext())
					goto _error;
				p =_iter->next();
				if (p->protocolNumber() != 126)
					goto _error;
				switch (newId)
				{
					case 0:
						_iter->remove();
						delete p;
						break;

					case 127:
						_iter->insert(OstProtocolManager.createProtocol(
								126, mpStream));
						break;
					default:
						goto _error;
				}
				break;

			case 127:
				if (!_iter->hasNext())
					goto _error;
				p =_iter->next();
				if (p->protocolNumber() != 126)
					goto _error;
				p =_iter->next();
				if (p->protocolNumber() != 126)
					goto _error;
				switch (newId)
				{
					case 0:
						_iter->previous();
						_iter->previous();
						p =_iter->next();
						_iter->remove();
						delete p;
						p =_iter->next();
						_iter->remove();
						delete p;
						break;

					case 126:
						_iter->previous();
						_iter->previous();
						p =_iter->next();
						_iter->remove();
						delete p;
						break;
					default:
						goto _error;
				}
				break;
			default:
				goto _error;
		}
	}
		
	oldId = newId;
	return;

_error:
	qFatal("%s: unexpected incident", __FUNCTION__);
}

void StreamConfigDialog::updateL3Protocol(int newId)
{
	static int oldId = 0;
	AbstractProtocol *p;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		Q_ASSERT(newId != -1);

		if (!skipProtocols(2))
			goto _error;
		
		switch (oldId)
		{
			case 0:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case -1:
			default:
				if (!_iter->hasNext())
					goto _error;

				p =_iter->next();

				if ((oldId != -1) && (p->protocolNumber() == (quint32) newId))
					goto _error;

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

_error:
	qFatal("%s: unexpected incident", __FUNCTION__);
}

void StreamConfigDialog::updateL4Protocol(int newId)
{
	static int oldId = 0;
	AbstractProtocol *p;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		Q_ASSERT(newId != -1);

		if (!skipProtocols(3))
			goto _error;
		
		switch (oldId)
		{
			case 0:
				_iter->insert(OstProtocolManager.createProtocol(
						newId, mpStream));
				break;

			case -1:
			default:
				if (!_iter->hasNext())
					goto _error;

				p =_iter->next();

				if ((oldId != -1) && (p->protocolNumber() == (quint32) newId))
					goto _error;

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

_error:
	qFatal("%s: unexpected incident", __FUNCTION__);
}

void StreamConfigDialog::updatePayloadProtocol(int newId)
{
	static int oldId = 52;
	AbstractProtocol *p;

	qDebug("%s:old id = %d new id = %d upd? = %d", __FUNCTION__, oldId, newId,
			isUpdateInProgress);

	if (oldId == newId)
		return; // Nothing to be done

	if (!isUpdateInProgress)
	{
		Q_ASSERT(newId != 0);
		Q_ASSERT(newId != -1);

		if (!skipProtocols(4))
			goto _error;
		
		if (!_iter->hasNext())
			goto _error;

		p =_iter->next();

		_iter->setValue(OstProtocolManager.createProtocol(
				newId, mpStream));
		delete p;
		while (_iter->hasNext())
		{

			p = _iter->next();
			_iter->remove();
			delete p;
		}
	}

	oldId = newId;
	return;

_error:
	qFatal("%s: unexpected incident", __FUNCTION__);
}

void StreamConfigDialog::updateSelectProtocolsSimpleWidget()
{
#define CHK(p) (_iter->hasNext() && _iter->peekNext()->protocolNumber() == p)

	qDebug("%s", __FUNCTION__);

	isUpdateInProgress = true;

	_iter->toFront();

	//
	// We expect first protocol to be 'mac' usually
	//
	if (CHK(51)) // Mac
	{
		_iter->next();
	}
	else
	{
		rbFtOther->setChecked(true);
		goto _done;
	}


	//
	// Check for "VLAN"
	//
	if (CHK(126)) // VLAN
	{
		_iter->next();
		if (CHK(126)) // VLAN
		{
			rbVlanDouble->setChecked(true);
			updateVlanProtocol(127);
			_iter->next();
		}
		else
		{
			rbVlanSingle->setChecked(true);
			updateVlanProtocol(126);
		}
	}
	else
	{
		rbVlanNone->setChecked(true);
		updateVlanProtocol(0);
	}


	//
	// Identify and set "FrameType"
	//
	if (CHK(52)) // Payload
	{
		rbFtNone->click();
		goto _payload;
	}
	else if (CHK(121)) // Eth2
	{
		rbFtEthernet2->click();
		_iter->next();
	}
	else if (CHK(122)) // 802.3 RAW
	{
		_iter->next();
		if (CHK(123)) // 802.3 LLC
		{
			_iter->next();
			if (CHK(124)) // SNAP
			{
				rbFtLlcSnap->click();
				_iter->next();
			}
			else
			{
				rbFt802Dot3Llc->click();
			}
		}
		else
		{
			rbFt802Dot3Raw->click();
		}
	}
	else
	{
		rbFtOther->setChecked(true);
		goto _done;
	}


	// 
	// --- L3 ---
	//
	if (CHK(130)) // IP4
	{
		rbL3Ipv4->click();
		_iter->next();
	}
	else if (CHK(131)) // ARP
	{
		rbL3Arp->click();
		_iter->next();
	}
	else if (CHK(52)) // Payload
	{
		rbL3None->click();
		goto _payload;
	}
	else
	{
		rbL3Other->setChecked(true);
		goto _done;
	}

	//
	// --- L4 ---
	//
	if (!_iter->hasNext())
	{
		rbL4Other->setChecked(true);
		goto _done;
	}
	else if (CHK(140)) // TCP
	{
		rbL4Tcp->click();
		_iter->next();
	}
	else if (CHK(141)) // UDP
	{
		rbL4Udp->click();
		_iter->next();
	}
	else if (CHK(142)) // ICMP
	{
		rbL4Icmp->click();
		_iter->next();
	}
	else if (CHK(143)) // IGMP
	{
		rbL4Igmp->click();
		_iter->next();
	}
	else if (CHK(52)) // Payload
	{
		rbL4None->click();
		goto _payload;
	}
	else
	{
		rbL4Other->setChecked(true);
		goto _done;
	}

_payload:
	//
	// Payload
	//
	if (CHK(52)) // Payload
	{
		rbPayloadPattern->click();
		_iter->next();
	}

	// If there is any protocol beyond "data pattern" ...
	if (_iter->hasNext())
		rbPayloadOther->setChecked(true);

_done:
	// If any "other" protocols are checked, QButtonGroup signals
	// are not triggered since the "other" radioButton's are disabled
    //	- so update manually
	if (rbFtOther->isChecked())
		updateFrameTypeProtocol(-1);
	if (rbL3Other->isChecked())
		updateL3Protocol(-1);
	if (rbL4Other->isChecked())
		updateL4Protocol(-1);
	if (rbPayloadOther->isChecked())
		updatePayloadProtocol(-1);
	isUpdateInProgress = false;

	return;
#undef CHK
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

