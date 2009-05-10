#include <QHostAddress>
#include "streamconfigdialog.h"
#include "stream.h"

#include "modeltest.h"

int StreamConfigDialog::lastTopLevelTabIndex = 0;
int StreamConfigDialog::lastProtoTabIndex = 0;

StreamConfigDialog::StreamConfigDialog(Port &port, uint streamIndex,
	QWidget *parent) : QDialog (parent), mPort(port)
{
	OstProto::Stream s;
	mCurrentStreamIndex = streamIndex;
	mpStream = new Stream;
	mPort.streamByIndex(mCurrentStreamIndex)->protoDataCopyInto(s);
	mpStream->protoDataCopyFrom(s);

	setupUi(this);
	setupUiExtra();

	connect(bgFrameType, SIGNAL(buttonClicked(int)),
		this, SLOT(updateSelectedProtocols()));
	connect(bgL3Proto, SIGNAL(buttonClicked(int)),
		this, SLOT(updateSelectedProtocols()));
	connect(bgL4Proto, SIGNAL(buttonClicked(int)),
		this, SLOT(updateSelectedProtocols()));
	
	// Time to play match the signals and slots!
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Enable/Disable L3 Protocol Choices for FT None
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Enable/Disable L3 Protocol Choices for FT Ethernet2
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Force L3 = None if FT = 802.3 Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Force L3 = None if FT = 802.3 LLC (to ensure a valid L3 is selected)
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Llc
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802.3 LLC SNAP
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Enable/Disable L4 Protocol Choices for L3 Protocol None
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4None, SLOT(setEnabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Icmp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Igmp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Tcp, SLOT(setDisabled(bool)));
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4Udp, SLOT(setDisabled(bool)));

	// Force L4 Protocol = None if L3 Protocol is set to None
	connect(rbL3None, SIGNAL(toggled(bool)), rbL4None, SLOT(setChecked(bool)));

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
	connect(rbL3Arp, SIGNAL(toggled(bool)), rbL4None, SLOT(setChecked(bool)));

	LoadCurrentStream();
	mpPacketModel = new PacketModel(QList<AbstractProtocol*>(), this);
	tvPacketTree->setModel(mpPacketModel);
	mpPacketModelTester = new ModelTest(mpPacketModel);
	tvPacketTree->header()->hide();
	vwPacketDump->setModel(mpPacketModel);
	vwPacketDump->setSelectionModel(tvPacketTree->selectionModel());

	// TODO(MED):
	//! \todo Implement then enable these protocols
	rbL3Arp->setHidden(true);
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
	if (twProto->isTabEnabled(lastProtoTabIndex))
		twProto->setCurrentIndex(lastProtoTabIndex);
}

void StreamConfigDialog::setupUiExtra()
{
	QRegExp reHex2B("[0-9,a-f,A-F]{1,4}");
	QRegExp reHex4B("[0-9,a-f,A-F]{1,8}");
	QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

	// Add the Payload widget to the dialog
	{
		QGridLayout		*layout;

		layout = static_cast<QGridLayout*>(twTopLevel->widget(0)->layout());

		layout->addWidget(mpStream->protocol(52)->configWidget(), 0, 1);
		qDebug("setupUi wgt = %p", mpStream->protocol(52)->configWidget());
	}

	// ---- Setup default stuff that cannot be done in designer ----
	bgFrameType = new QButtonGroup();
	foreach(QRadioButton *btn, gbFrameType->findChildren<QRadioButton*>())
		bgFrameType->addButton(btn);

	bgL3Proto = new QButtonGroup();
	foreach(QRadioButton *btn, gbL3Proto->findChildren<QRadioButton*>())
		bgL3Proto->addButton(btn);

	bgL4Proto = new QButtonGroup();
	foreach(QRadioButton *btn, gbL4Proto->findChildren<QRadioButton*>())
		bgL4Proto->addButton(btn);

	//twProto->setTabEnabled(2, FALSE);
	//twProto->setTabEnabled(3, FALSE);

	/*
	** Setup Validators
	*/	
	// Meta Data
	lePktLen->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN, this));
	
	// L2 Ethernet
#if 0 // Proto FW
	leDstMac->setValidator(new QRegExpValidator(reMac, this));
	leSrcMac->setValidator(new QRegExpValidator(reMac, this));
	leDstMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
	leSrcMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
	leCvlanTpid->setValidator(new QRegExpValidator(reHex2B, this));
	leSvlanTpid->setValidator(new QRegExpValidator(reHex2B, this));
	//leEtherType->setValidator(new QRegExpValidator(reHex2B, this));
#endif

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

	// Remove payload data widget so that it is not deleted when this object
	// is destroyed
	{
		QLayout *layout = twTopLevel->widget(0)->layout();
		if (layout)
		{
			layout->removeWidget(mpStream->protocol(52)->configWidget());
			mpStream->protocol(52)->configWidget()->setParent(0);
		}
	}

	// Remove any existing widget on the L2-L4 Tabs lest they are deleted
	// when this object is destoryed
	for (int i = 1; i <= 3; i++)
	{
		QLayout *layout = twProto->widget(i)->layout();
		if (layout)
		{
			QLayoutItem *child;
			while ((child = layout->takeAt(0)) != 0)
			{
				Q_ASSERT(child->widget() != 0);
				// Don't delete the child widget - reparent it
				child->widget()->setParent(0);
			} 
			delete layout;
		}
	}

	delete bgFrameType;
	delete bgL3Proto;
	delete bgL4Proto;

	delete mpStream;
}

void StreamConfigDialog::updateSelectedProtocols()
{
	mSelectedProtocols.clear();

	// FIXME: Hardcoded numbers!

	// Mac
	mSelectedProtocols.append(51);

	if (rbFtEthernet2->isChecked())
		mSelectedProtocols.append(121);
	else if (rbFt802Dot3Raw->isChecked())
		mSelectedProtocols.append(122);
	else if (rbFt802Dot3Llc->isChecked())
	{
		mSelectedProtocols.append(122);
		mSelectedProtocols.append(123);
	}
	else if (rbFtLlcSnap->isChecked())
	{
		mSelectedProtocols.append(122);
		mSelectedProtocols.append(123);
		mSelectedProtocols.append(124);
	}

	if (rbL3Ipv4->isChecked())
		mSelectedProtocols.append(130);
	else if (rbL3Arp->isChecked())
		mSelectedProtocols.append(131);

	if (rbL4Tcp->isChecked())
		mSelectedProtocols.append(140);
	else if (rbL4Udp->isChecked())
		mSelectedProtocols.append(141);
	else if (rbL4Icmp->isChecked())
		mSelectedProtocols.append(142);
	else if (rbL4Igmp->isChecked())
		mSelectedProtocols.append(143);

	// Payload
	mSelectedProtocols.append(52);

	mpStream->setFrameProtocol(mSelectedProtocols);
	mpStream->storeProtocolWidgets();
	mpStream->loadProtocolWidgets();
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

void StreamConfigDialog::on_twTopLevel_currentChanged(int index)
{
	QList<AbstractProtocol*> protoList;

	// We only process the "Packet View" tab
	if (index != 2)
		return;

	updateSelectedProtocols();
	foreach(int i, mSelectedProtocols)
		if (mpStream->protocol(i))
			protoList.append(mpStream->protocol(i));
	
	mpPacketModel->setSelectedProtocols(protoList);
	StoreCurrentStream(mpStream);
}

void StreamConfigDialog::on_twProto_currentChanged(int index)
{
	QLayout *layout;
	QList<QWidget*> wl;

	// We need to process only indices 1-3 i.e. the L2, L3 and L4 tabs
	if ((index < 1) || (index > 3))
		return;

	// Remove any existing widget on the activated tab
	layout = twProto->widget(index)->layout();
	if (layout)
	{
		QLayoutItem *child;
		while ((child = layout->takeAt(0)) != 0)
		{
			Q_ASSERT(child->widget() != 0);
			// Don't delete the child widget - reparent it
			child->widget()->setParent(0);
		} 
		delete layout;
	}

	// FIXME: protocol id hardcodings
	switch(index)
	{
		case 1: // L2
			wl.append(mpStream->protocol("mac")->configWidget());
			if (rbFtEthernet2->isChecked())
				wl.append(mpStream->protocol("eth2")->configWidget());
			else if (rbFt802Dot3Raw->isChecked())
				wl.append(mpStream->protocol("dot3")->configWidget());
			else if (rbFt802Dot3Llc->isChecked())
			{
				wl.append(mpStream->protocol("dot3")->configWidget());
				wl.append(mpStream->protocol("llc")->configWidget());
			}
			else if (rbFtLlcSnap->isChecked())
			{
				wl.append(mpStream->protocol("dot3")->configWidget());
				wl.append(mpStream->protocol("llc")->configWidget());
				wl.append(mpStream->protocol("snap")->configWidget());
			}
			break;
		case 2: // L3
			if (rbL3Ipv4->isChecked())
				wl.append(mpStream->protocol("ip4")->configWidget());
			break;
		case 3: // L4
			if (rbL4Tcp->isChecked())
				wl.append(mpStream->protocol("tcp")->configWidget());
			else if (rbL4Udp->isChecked())
				wl.append(mpStream->protocol("udp")->configWidget());
			break;
	}

	if (wl.size()) 
	{
		layout = new QVBoxLayout;

		for (int i=0; i < wl.size(); i++)
			layout->addWidget(wl.at(i));

		twProto->widget(index)->setLayout(layout);
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
		int i;

		qDebug("Selected Protocols.size() = %d", mSelectedProtocols.size());
		mSelectedProtocols = mpStream->frameProtocol();
		qDebug("Selected Protocols.size() = %d", mSelectedProtocols.size());
		for (i = 0; i < mSelectedProtocols.size(); i++)
			qDebug("%d: %d", i, mSelectedProtocols.at(i));

		Q_ASSERT(mSelectedProtocols.size() >= 2); // Mac + Payload: mandatory

		i = 0;
		Q_ASSERT(mSelectedProtocols.at(i) == 51); // Mac
		i++;

		if (mSelectedProtocols.at(i) == 52) // Payload
		{
			i++;
			goto _proto_parse_done;
		}
		else if (mSelectedProtocols.at(i) == 121) // Eth2
		{
			rbFtEthernet2->setChecked(true);
			i++;
		}
		else if (mSelectedProtocols.at(i) == 122) // 802.3 RAW
		{
			if ((mSelectedProtocols.size() > (i+1)) &&
					(mSelectedProtocols.at(i+1) == 123)) // 802.3 LLC
			{
				if ((mSelectedProtocols.size() > (i+2)) && 
						(mSelectedProtocols.at(i+2) == 124)) // SNAP
				{
					rbFtLlcSnap->setChecked(true);
					i+=3;
				}
				else
				{
					rbFt802Dot3Llc->setChecked(true);
					i+=2;
				}
			}
			else
			{
				rbFt802Dot3Raw->setChecked(true);
				i++;
			}
		}
		else
			rbFtNone->setChecked(true);

		// L3
		if (mSelectedProtocols.at(i) == 52) // Payload
		{
			i++;
			goto _proto_parse_done;
		}
		else if (mSelectedProtocols.at(i) == 130) // IP4
		{
			rbL3Ipv4->setChecked(true);
			i++;
		}
	   	else if (mSelectedProtocols.at(i) == 131) // ARP
		{
			rbL3Arp->setChecked(true);
			i++;
		}
		else
			rbL3None->setChecked(true);

		if (i == mSelectedProtocols.size())
			goto _proto_parse_done;

		// L4
		if (mSelectedProtocols.at(i) == 52) // Payload
		{
			i++;
			goto _proto_parse_done;
		}
		else if (mSelectedProtocols.at(i) == 140) // TCP
		{
			rbL4Tcp->setChecked(true);
			i++;
		}
		else if (mSelectedProtocols.at(i) == 141) // UDP
		{
			rbL4Udp->setChecked(true);
			i++;
		}
		else if (mSelectedProtocols.at(i) == 142) // ICMP
		{
			rbL4Icmp->setChecked(true);
			i++;
		}
		else if (mSelectedProtocols.at(i) == 143) // IGMP
		{
			rbL4Igmp->setChecked(true);
			i++;
		}
		else
			rbL4None->setChecked(true);

		Q_ASSERT(mSelectedProtocols.at(i) == 52); // Payload
		i++;

_proto_parse_done:
		Q_ASSERT(i == mSelectedProtocols.size());

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
}

void StreamConfigDialog::StoreCurrentStream(Stream *pStream)
{
	QString	str;
	bool	isOk;

	qDebug("storing pStream %p", pStream);

	// Meta Data
	pStream->setLenMode((Stream::FrameLengthMode) cmbPktLenMode->currentIndex());
	pStream->setFrameLen(lePktLen->text().toULong(&isOk));
	pStream->setFrameLenMin(lePktLenMin->text().toULong(&isOk));
	pStream->setFrameLenMax(lePktLenMax->text().toULong(&isOk));

	// Protocols
	{
		updateSelectedProtocols();
		pStream->setFrameProtocol(mSelectedProtocols);
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
	// Store dialog contents into stream
	StoreCurrentStream(mPort.streamByIndex(mCurrentStreamIndex));
	qDebug("stream stored");
	lastTopLevelTabIndex = twTopLevel->currentIndex();
	lastProtoTabIndex = twProto->currentIndex();
}

