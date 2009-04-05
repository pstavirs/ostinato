#include <QHostAddress>
#include "streamconfigdialog.h"
#include "stream.h"

#include "modeltest.h"

int StreamConfigDialog::lastTopLevelTabIndex = 0;
int StreamConfigDialog::lastProtoTabIndex = 0;

// TODO(HI): Write HexLineEdit::setNum() and num() and use it in 
// Load/Store stream methods

StreamConfigDialog::StreamConfigDialog(Port &port, uint streamIndex,
	QWidget *parent) : QDialog (parent), mPort(port)
{
	setupUi(this);
	setupUiExtra();

	// setupUi
	
	// Time to play match the signals and slots!
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Show/Hide FrameType related inputs for FT None
	connect(rbFtNone, SIGNAL(toggled(bool)), lblDsap, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), leDsap, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), lblSsap, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), leSsap, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), lblControl, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), leControl, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), lblOui, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), leOui, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), lblType, SLOT(setHidden(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), leType, SLOT(setHidden(bool)));

	// Enable/Disable L3 Protocol Choices for FT None
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFtNone, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Show/Hide FrameType related inputs for FT Ethernet2
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), lblDsap, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), leDsap, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), lblSsap, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), leSsap, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), lblControl, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), leControl, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), lblOui, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), leOui, SLOT(setHidden(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), lblType, SLOT(setVisible(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), leType, SLOT(setVisible(bool)));

	// Enable/Disable L3 Protocol Choices for FT Ethernet2
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtEthernet2, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Show/Hide FrameType related inputs for FT 802.3 Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), lblDsap, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), leDsap, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), lblSsap, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), leSsap, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), lblControl, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), leControl, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), lblOui, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), leOui, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), lblType, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), leType, SLOT(setHidden(bool)));

	// Force L3 = None if FT = 802.3 Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Raw
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setDisabled(bool)));
	connect(rbFt802Dot3Raw, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Show/Hide FrameType related inputs for FT 802.3 LLC
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), lblDsap, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), leDsap, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), lblSsap, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), leSsap, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), lblControl, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), leControl, SLOT(setVisible(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), lblOui, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), leOui, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), lblType, SLOT(setHidden(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), leType, SLOT(setHidden(bool)));

	// Force L3 = None if FT = 802.3 LLC (to ensure a valid L3 is selected)
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3None, SLOT(setChecked(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802Dot3Llc
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFt802Dot3Llc, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setDisabled(bool)));

	// Show/Hide FrameType related inputs for FT 802.3 LLC SNAP
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblDsap, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leDsap, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblSsap, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leSsap, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblControl, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leControl, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblOui, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leOui, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblType, SLOT(setVisible(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leType, SLOT(setVisible(bool)));

	// Enable/Disable L3 Protocol Choices for FT 802.3 LLC SNAP
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3None, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Ipv4, SLOT(setEnabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), rbL3Arp, SLOT(setEnabled(bool)));

	// Enable/Disable FrameType related inputs for FT 802.3 LLC SNAP
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblDsap, SLOT(setDisabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leDsap, SLOT(setDisabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblSsap, SLOT(setDisabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leSsap, SLOT(setDisabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), lblControl, SLOT(setDisabled(bool)));
	connect(rbFtLlcSnap, SIGNAL(toggled(bool)), leControl, SLOT(setDisabled(bool)));

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

	//TODO: remove if not needed
#if 0
	// This set of 'clicks' is a hack to trigger signals at dialog creation
	// time so that a coherent 'set' is initialized
	// Actual stream values will be initialized by LoadCurrentStream()
	rbL3Ipv4->click();
	rbL3None->click();
	rbFtEthernet2->click();
	rbFtNone->click();
#endif

	//mpStreamList = streamList;
	mCurrentStreamIndex = streamIndex;
	LoadCurrentStream();
	mpPacketModel = new PacketModel(&mPort.streamByIndex(mCurrentStreamIndex),
		this);
	tvPacketTree->setModel(mpPacketModel);
	mpPacketModelTester = new ModelTest(mpPacketModel);
	tvPacketTree->header()->hide();
	vwPacketDump->setModel(mpPacketModel);
	vwPacketDump->setSelectionModel(tvPacketTree->selectionModel());



	// TODO(MED):
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

	// ---- Setup default stuff that cannot be done in designer ----

	// Since the dialog defaults are FT = None, L3 = None, L4 = None;
  	// hide associated input fields since it can't be done in Designer
	lblDsap->setHidden(true);
	leDsap->setHidden(true);
	lblSsap->setHidden(true);
	leSsap->setHidden(true);
	lblControl->setHidden(true);
	leControl->setHidden(true);
	lblOui->setHidden(true);
	leOui->setHidden(true);
	lblType->setHidden(true);
	leType->setHidden(true);

	twProto->setTabEnabled(2, FALSE);
	twProto->setTabEnabled(3, FALSE);

	/*
	** Setup Validators
	*/	
	// Meta Data
	lePktLen->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN, this));
	
	// L2 Ethernet
	leDstMac->setValidator(new QRegExpValidator(reMac, this));
	leSrcMac->setValidator(new QRegExpValidator(reMac, this));
	leDstMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
	leSrcMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
	leCvlanTpid->setValidator(new QRegExpValidator(reHex2B, this));
	leSvlanTpid->setValidator(new QRegExpValidator(reHex2B, this));
	//leEtherType->setValidator(new QRegExpValidator(reHex2B, this));

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
}

void StreamConfigDialog::on_cmbPatternMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed Word")
	{
		lePattern->setEnabled(true);
	}
	else if (mode == "Increment Byte")
	{
		lePattern->setDisabled(true);
	}
	else if (mode == "Decrement Byte")
	{
		lePattern->setDisabled(true);
	}
	if (mode == "Random")
	{
		lePattern->setDisabled(true);
	}
	else
	{
		qWarning("Unhandled/Unknown PatternMode = %s", mode.toAscii().data());
	}
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


void StreamConfigDialog::on_cmbDstMacMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
	{
		leDstMacCount->setEnabled(false);
		leDstMacStep->setEnabled(false);
	}
	else
	{
		leDstMacCount->setEnabled(true);
		leDstMacStep->setEnabled(true);
	}
}

void StreamConfigDialog::on_cmbSrcMacMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
	{
		leSrcMacCount->setEnabled(false);
		leSrcMacStep->setEnabled(false);
	}
	else
	{
		leSrcMacCount->setEnabled(true);
		leSrcMacStep->setEnabled(true);
	}
}

void StreamConfigDialog::on_cmbIpSrcAddrMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
	{
		leIpSrcAddrCount->setDisabled(true);
		leIpSrcAddrMask->setDisabled(true);
	}
	else
	{
		leIpSrcAddrCount->setEnabled(true);
		leIpSrcAddrMask->setEnabled(true);
	}
}

void StreamConfigDialog::on_cmbIpDstAddrMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
	{
		leIpDstAddrCount->setDisabled(true);
		leIpDstAddrMask->setDisabled(true);
	}
	else
	{
		leIpDstAddrCount->setEnabled(true);
		leIpDstAddrMask->setEnabled(true);
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

void StreamConfigDialog::on_rbFtNone_toggled(bool checked)
{
}

void StreamConfigDialog::on_rbFtEthernet2_toggled(bool checked)
{
}

void StreamConfigDialog::on_rbFt802Dot3Raw_toggled(bool checked)
{
}

void StreamConfigDialog::on_rbFt802Dot3Llc_toggled(bool checked)
{
}

void StreamConfigDialog::on_rbFtLlcSnap_toggled(bool checked)
{
	if (checked)
	{
		leDsap->setText("AA");
		leSsap->setText("AA");
		leControl->setText("03");
	}
}

void StreamConfigDialog::on_rbL3Ipv4_toggled(bool checked)
{
	if (checked)
	{
		swL3Proto->setCurrentIndex(0);
		twProto->setTabEnabled(2, TRUE);
		twProto->setTabText(2, "L3 (IPv4)");
		leType->setText("08 00");
	}
	else
	{
		twProto->setTabEnabled(2, FALSE);
		twProto->setTabText(2, "L3");
	}
}

void StreamConfigDialog::on_rbL3Arp_toggled(bool checked)
{
	if (checked)
	{
		swL3Proto->setCurrentIndex(1);
		twProto->setTabEnabled(2, TRUE);
		twProto->setTabText(2, "L3 (ARP)");
		leType->setText("08 06");
	}
	else
	{
		twProto->setTabEnabled(2, FALSE);
		twProto->setTabText(2, "L3");
	}
}

void StreamConfigDialog::on_rbL4Icmp_toggled(bool checked)
{
	QString str;

	if (checked)
	{
		swL4Proto->setCurrentIndex(2);
		twProto->setTabEnabled(3, TRUE);
		twProto->setTabText(3, "L4 (ICMP)");
		leIpProto->setText(uintToHexStr(IP_PROTO_ICMP, str, 1));
	}
	else
	{
		twProto->setTabEnabled(3, FALSE);
		twProto->setTabText(3, "L4");
	}
}

void StreamConfigDialog::on_rbL4Igmp_toggled(bool checked)
{
	QString str;

	if (checked)
	{
		swL4Proto->setCurrentIndex(3);
		twProto->setTabEnabled(3, TRUE);
		twProto->setTabText(3, "L4 (IGMP)");
		leIpProto->setText(uintToHexStr(IP_PROTO_IGMP, str, 1));
	}
	else
	{
		twProto->setTabEnabled(3, FALSE);
		twProto->setTabText(3, "L4");
	}
}

void StreamConfigDialog::on_rbL4Tcp_toggled(bool checked)
{
	QString str;

	if (checked)
	{
		swL4Proto->setCurrentIndex(0);
		twProto->setTabEnabled(3, TRUE);
		twProto->setTabText(3, "L4 (TCP)");
		leIpProto->setText(uintToHexStr(IP_PROTO_TCP, str, 1));
	}
	else
	{
		twProto->setTabEnabled(3, FALSE);
		twProto->setTabText(3, "L4");
	}
}

void StreamConfigDialog::on_rbL4Udp_toggled(bool checked)
{
	QString str;

	if (checked)
	{
		swL4Proto->setCurrentIndex(1);
		twProto->setTabEnabled(3, TRUE);
		twProto->setTabText(3, "L4 (UDP)");
		leIpProto->setText(uintToHexStr(IP_PROTO_UDP, str, 1));
	}
	else
	{
		twProto->setTabEnabled(3, FALSE);
		twProto->setTabText(3, "L4");
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

QString & uintToHexStr(quint64 num, QString &hexStr, quint8 octets) 
{
	int i;
	QChar	zero('0');

	hexStr = "";

	for (i = octets; i > 0; i--)
	{
		ushort byte;
		QString str1 = "%1";
		QString str;

		byte = num & 0xff;
		str = str1.arg(byte, 2, 16, zero).append(' '); 
		hexStr.prepend(str);
		num = num >> 8;
	}

	return hexStr;
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
	Stream	*pStream = &mPort.streamByIndex(mCurrentStreamIndex);
	QString	str;

	qDebug("loading pStream %p", pStream);

	// Meta Data
	{
		cmbPatternMode->setCurrentIndex(pStream->patternMode());
		lePattern->setText(uintToHexStr(pStream->pattern(), str, 4));

		cmbPktLenMode->setCurrentIndex(pStream->lenMode());
		lePktLen->setText(str.setNum(pStream->frameLen()));
		lePktLenMin->setText(str.setNum(pStream->frameLenMin()));
		lePktLenMax->setText(str.setNum(pStream->frameLenMax()));
	}

	// Protocols
	{
		qDebug("ft = %d\n", pStream->frameType());
		switch(pStream->frameType())
		{
			case Stream::e_ft_none:
				rbFtNone->setChecked(TRUE);
				break;
			case Stream::e_ft_eth_2:
				rbFtEthernet2->setChecked(TRUE);
				break;
			case Stream::e_ft_802_3_raw:
				rbFt802Dot3Raw->setChecked(TRUE);
				break;
			case Stream::e_ft_802_3_llc:
				rbFt802Dot3Llc->setChecked(TRUE);
				break;
			case Stream::e_ft_snap:
				rbFtLlcSnap->setChecked(TRUE);
				break;
		}

		leDsap->setText(uintToHexStr(pStream->llc()->dsap(), str, 1));
		leSsap->setText(uintToHexStr(pStream->llc()->ssap(), str, 1));
		leControl->setText(uintToHexStr(pStream->llc()->ctl(), str, 1));
		leOui->setText(uintToHexStr(pStream->snap()->oui(), str, 3));

		leType->setText(uintToHexStr(pStream->eth2()->type(), str, 2));

		switch(pStream->l3Proto())
		{
		case Stream::e_l3_none:
			rbL3None->setChecked(true);
			break;
		case Stream::e_l3_ip:
			rbL3Ipv4->setChecked(true);
			break;
		case Stream::e_l3_arp:
			rbL3Arp->setChecked(true);
			break;
		default:
			qDebug("%s: unknown L3 Protocol %d", __FUNCTION__,
				pStream->l3Proto());
		}

		switch(pStream->l4Proto())
		{
		case Stream::e_l4_none:
			rbL4None->setChecked(true);
			break;
		case Stream::e_l4_tcp:
			rbL4Tcp->setChecked(true);
			break;
		case Stream::e_l4_udp:
			rbL4Udp->setChecked(true);
			break;
		case Stream::e_l4_icmp:
			rbL4Icmp->setChecked(true);
			break;
		case Stream::e_l4_igmp:
			rbL4Igmp->setChecked(true);
			break;
		default:
			qDebug("%s: unknown l4 Protocol %d", __FUNCTION__,
				pStream->l4Proto());
		}
	}

	// L2
	{
		// L2 | Ethernet
		{
			leDstMac->setText(uintToHexStr(pStream->mac()->dstMac(), str, 6));
			cmbDstMacMode->setCurrentIndex(pStream->mac()->dstMacMode());
			leDstMacCount->setText(str.setNum(pStream->mac()->dstMacCount()));
			leDstMacStep->setText(str.setNum(pStream->mac()->dstMacStep()));

			leSrcMac->setText(uintToHexStr(pStream->mac()->srcMac(), str, 6));
			cmbSrcMacMode->setCurrentIndex(pStream->mac()->srcMacMode());
			leSrcMacCount->setText(str.setNum(pStream->mac()->srcMacCount()));
			leSrcMacStep->setText(str.setNum(pStream->mac()->srcMacStep()));

			{
				VlanProtocol			*vlan = pStream->vlan();
				VlanProtocol::VlanFlags	f;

				cmbCvlanPrio->setCurrentIndex(vlan->cvlanPrio());
				cmbCvlanCfi->setCurrentIndex(vlan->cvlanCfi());
				leCvlanId->setText(str.setNum(vlan->cvlanId()));
				leCvlanTpid->setText(str.setNum(vlan->ctpid()));
				cbCvlanTpidOverride->setChecked(vlan->vlanFlags().testFlag(
					VlanProtocol::VlanCtpidOverride));
				gbCvlan->setChecked(vlan->vlanFlags().testFlag(
					VlanProtocol::VlanCvlanTagged));

				cmbSvlanPrio->setCurrentIndex(vlan->svlanPrio());
				cmbSvlanCfi->setCurrentIndex(vlan->svlanCfi());
				leSvlanId->setText(str.setNum(vlan->svlanId()));
				leSvlanTpid->setText(str.setNum(vlan->stpid()));
				cbSvlanTpidOverride->setChecked(vlan->vlanFlags().testFlag(
					VlanProtocol::VlanStpidOverride));
				gbSvlan->setChecked(vlan->vlanFlags().testFlag(
					VlanProtocol::VlanSvlanTagged));
			}
		}
	}

	// L3
	{
		// L3 | IP
		{
			leIpVersion->setText(str.setNum(pStream->ip()->ver()));
			cbIpVersionOverride->setChecked(
				pStream->ip()->ipFlags().testFlag(IpProtocol::IpOverrideVersion));
			leIpHdrLen->setText(str.setNum(pStream->ip()->hdrLen()));
			cbIpHdrLenOverride->setChecked(
				pStream->ip()->ipFlags().testFlag(IpProtocol::IpOverrideHdrLen));
			
			leIpTos->setText(uintToHexStr(pStream->ip()->tos(), str, 1));

			leIpLength->setText(str.setNum(pStream->ip()->totLen()));
			cbIpLengthOverride->setChecked(
				pStream->ip()->ipFlags().testFlag(IpProtocol::IpOverrideTotLen));

			leIpId->setText(uintToHexStr(pStream->ip()->id(), str, 2));
			leIpFragOfs->setText(str.setNum(pStream->ip()->fragOfs()));
			cbIpFlagsDf->setChecked((pStream->ip()->flags() & IP_FLAG_DF) > 0);
			cbIpFlagsMf->setChecked((pStream->ip()->flags() & IP_FLAG_MF) > 0);

			leIpTtl->setText(str.setNum(pStream->ip()->ttl()));
			leIpProto->setText(uintToHexStr(pStream->ip()->proto(), str, 1));

			leIpCksum->setText(uintToHexStr(pStream->ip()->cksum(), str, 2));
			cbIpCksumOverride->setChecked(
				pStream->ip()->ipFlags().testFlag(IpProtocol::IpOverrideCksum));

			leIpSrcAddr->setText(QHostAddress(pStream->ip()->srcIp()).toString());
			cmbIpSrcAddrMode->setCurrentIndex(pStream->ip()->srcIpMode());
			leIpSrcAddrCount->setText(str.setNum(pStream->ip()->srcIpCount()));
			leIpSrcAddrMask->setText(QHostAddress(pStream->ip()->srcIpMask()).toString());

			leIpDstAddr->setText(QHostAddress(pStream->ip()->dstIp()).toString());
			cmbIpDstAddrMode->setCurrentIndex(pStream->ip()->dstIpMode());
			leIpDstAddrCount->setText(str.setNum(pStream->ip()->dstIpCount()));
			leIpDstAddrMask->setText(QHostAddress(pStream->ip()->dstIpMask()).toString());
		}

		// L3 | ARP
		{
			// TODO(LOW)
		}
	}

	// L4
	{
		// L4 | TCP
		{
			leTcpSrcPort->setText(str.setNum(pStream->tcp()->srcPort()));
			leTcpDstPort->setText(str.setNum(pStream->tcp()->dstPort()));

			leTcpSeqNum->setText(str.setNum(pStream->tcp()->seqNum()));
			leTcpAckNum->setText(str.setNum(pStream->tcp()->ackNum()));

			leTcpHdrLen->setText(str.setNum(pStream->tcp()->hdrLen()));
			cbTcpHdrLenOverride->setChecked((pStream->tcp()->tcpFlags().
				testFlag(TcpProtocol::TcpOverrideHdrLen)));

			leTcpWindow->setText(str.setNum(pStream->tcp()->window()));

			leTcpCksum->setText(str.setNum(pStream->tcp()->cksum()));
			cbTcpCksumOverride->setChecked((pStream->tcp()->tcpFlags().
				testFlag(TcpProtocol::TcpOverrideCksum)));

			leTcpUrgentPointer->setText(str.setNum(pStream->tcp()->urgPtr()));

			cbTcpFlagsUrg->setChecked((pStream->tcp()->flags() & TCP_FLAG_URG) > 0);
			cbTcpFlagsAck->setChecked((pStream->tcp()->flags() & TCP_FLAG_ACK) > 0);
			cbTcpFlagsPsh->setChecked((pStream->tcp()->flags() & TCP_FLAG_PSH) > 0);
			cbTcpFlagsRst->setChecked((pStream->tcp()->flags() & TCP_FLAG_RST) > 0);
			cbTcpFlagsSyn->setChecked((pStream->tcp()->flags() & TCP_FLAG_SYN) > 0);
			cbTcpFlagsFin->setChecked((pStream->tcp()->flags() & TCP_FLAG_FIN) > 0);
		}

		// L4 | UDP
		{
			leUdpSrcPort->setText(str.setNum(pStream->udp()->srcPort()));
			leUdpDstPort->setText(str.setNum(pStream->udp()->dstPort()));

			leUdpLength->setText(str.setNum(pStream->udp()->totLen()));
			cbUdpLengthOverride->setChecked((pStream->udp()->udpFlags().
				testFlag(UdpProtocol::UdpOverrideTotLen)));


			leUdpCksum->setText(str.setNum(pStream->udp()->cksum()));
			cbUdpCksumOverride->setChecked((pStream->udp()->udpFlags().
				testFlag(UdpProtocol::UdpOverrideCksum)));
		}

		// L4 | ICMP
		{
			// TODO(LOW)
		}

		// L4 | IGMP
		{
			// TODO(LOW)
		}
	}

	// Stream Control
	{
		switch (pStream->sendUnit())
		{
		case Stream::e_su_packets:
			rbSendPackets->setChecked(true);
			break;
		case Stream::e_su_bursts:
			rbSendBursts->setChecked(true);
			break;
		default:
			qWarning("Unhandled sendUnit = %d\n", pStream->sendUnit());
		}

		switch (pStream->sendMode())
		{
		case Stream::e_sm_fixed:
			rbModeFixed->setChecked(true);
			break;
		case Stream::e_sm_continuous:
			rbModeContinuous->setChecked(true);
			break;
		default:
			qWarning("Unhandled sendMode = %d\n", pStream->sendMode());
		}

		switch(pStream->nextWhat())
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
			qWarning("Unhandled nextAction = %d\n", pStream->nextWhat());
		}

		leNumPackets->setText(QString().setNum(pStream->numPackets()));
		leNumBursts->setText(QString().setNum(pStream->numBursts()));
		lePacketsPerBurst->setText(QString().setNum(pStream->burstSize()));
		lePacketsPerSec->setText(QString().setNum(pStream->packetRate()));
		leBurstsPerSec->setText(QString().setNum(pStream->burstRate()));
		// TODO(MED): Change this when we support goto to specific stream
		leStreamId->setText(QString("0"));
	}
}

void StreamConfigDialog::StoreCurrentStream()
{
	Stream	*pStream = &mPort.streamByIndex(mCurrentStreamIndex);
	QString	str;
	bool	isOk;

	qDebug("storing pStream %p", pStream);

	// Meta Data
	pStream->setPatternMode((Stream::DataPatternMode) cmbPatternMode->currentIndex());
	pStream->setPattern(lePattern->text().remove(QChar(' ')).toULong(&isOk, 16));

	pStream->setLenMode((Stream::FrameLengthMode) cmbPktLenMode->currentIndex());
	pStream->setFrameLen(lePktLen->text().toULong(&isOk));
	pStream->setFrameLenMin(lePktLenMin->text().toULong(&isOk));
	pStream->setFrameLenMax(lePktLenMax->text().toULong(&isOk));

	// Protocols
	{
		if (rbFtNone->isChecked()) 
			pStream->setFrameType(Stream::e_ft_none);
		else if (rbFtEthernet2->isChecked())
			pStream->setFrameType(Stream::e_ft_eth_2);
		else if (rbFt802Dot3Raw->isChecked())
			pStream->setFrameType(Stream::e_ft_802_3_raw);
		else if (rbFt802Dot3Llc->isChecked())
			pStream->setFrameType(Stream::e_ft_802_3_llc);
		else if (rbFtLlcSnap->isChecked())
			pStream->setFrameType(Stream::e_ft_snap);
		qDebug("store ft(%d)\n", pStream->frameType());

		pStream->llc()->setDsap(leDsap->text().remove(QChar(' ')).toULong(&isOk, 16));
		pStream->llc()->setSsap(leSsap->text().remove(QChar(' ')).toULong(&isOk, 16));
		pStream->llc()->setCtl(leControl->text().remove(QChar(' ')).toULong(&isOk, 16));
		pStream->snap()->setOui(leOui->text().remove(QChar(' ')).toULong(&isOk, 16));
		pStream->eth2()->setType(leType->text().remove(QChar(' ')).toULong(&isOk, 16));

		if (rbL3None->isChecked())
			pStream->setL3Proto(Stream::e_l3_none);
		else if (rbL3Ipv4->isChecked())
			pStream->setL3Proto(Stream::e_l3_ip);
		else if (rbL3Arp->isChecked())
			pStream->setL3Proto(Stream::e_l3_arp);
		else
		{
			qCritical("No L3 Protocol??? Problem in Code!!!");
			pStream->setL3Proto(Stream::e_l3_none);
		}

		if (rbL4None->isChecked())
			pStream->setL4Proto(Stream::e_l4_none);
		else if (rbL4Tcp->isChecked())
			pStream->setL4Proto(Stream::e_l4_tcp);
		else if (rbL4Udp->isChecked())
			pStream->setL4Proto(Stream::e_l4_udp);
		else if (rbL4Icmp->isChecked())
			pStream->setL4Proto(Stream::e_l4_icmp);
		else if (rbL4Igmp->isChecked())
			pStream->setL4Proto(Stream::e_l4_igmp);
		else
		{
			qCritical("No L4 Protocol??? Problem in Code!!!");
			pStream->setL4Proto(Stream::e_l4_none);
		}
	}

	// L2
	{
		// L2 | Ethernet
		{
			qDebug("%s: LL dstMac = %llx", __FUNCTION__, 
				leDstMac->text().remove(QChar(' ')).toULongLong(&isOk, 16));
			pStream->mac()->setDstMac(
				leDstMac->text().remove(QChar(' ')).toULongLong(&isOk, 16));
#if 1
			qDebug("%s: dstMac = %llx", __FUNCTION__, 
					pStream->mac()->dstMac());
			qDebug("%s: dstMac = [%s] %d", __FUNCTION__, 
					leDstMac->text().toAscii().constData(), isOk);
#endif
			pStream->mac()->setDstMacMode(
				(MacProtocol::MacAddrMode) cmbDstMacMode->currentIndex());
			pStream->mac()->setDstMacCount(
				leDstMacCount->text().toULong(&isOk));
			pStream->mac()->setDstMacStep(
				leDstMacStep->text().toULong(&isOk));

			pStream->mac()->setSrcMac(
				leSrcMac->text().remove(QChar(' ')).toULongLong(&isOk, 16));
			qDebug("%s: srcMac = %llx", __FUNCTION__, 
					pStream->mac()->srcMac());
			qDebug("%s: srcMac = [%s] %d", __FUNCTION__, 
					leSrcMac->text().toAscii().constData(), isOk);
			pStream->mac()->setSrcMacMode(
				(MacProtocol::MacAddrMode) cmbSrcMacMode->currentIndex());
			pStream->mac()->setSrcMacCount(
				leSrcMacCount->text().toULong(&isOk));
			pStream->mac()->setSrcMacStep(

				leSrcMacStep->text().toULong(&isOk));

			{
				VlanProtocol			*vlan = pStream->vlan();
				VlanProtocol::VlanFlags	f = 0;

				vlan->setCvlanPrio(cmbCvlanPrio->currentIndex());
				vlan->setCvlanCfi(cmbCvlanCfi->currentIndex());
				vlan->setCvlanId(leCvlanId->text().toULong(&isOk));
				vlan->setCtpid(leCvlanTpid->text().remove(QChar(' ')).toULong(&isOk, 16));
				if (cbCvlanTpidOverride->isChecked())
					f |= VlanProtocol::VlanCtpidOverride;
				if (gbCvlan->isChecked())
					f |= VlanProtocol::VlanCvlanTagged;

				vlan->setSvlanPrio(cmbSvlanPrio->currentIndex());
				vlan->setSvlanCfi(cmbSvlanCfi->currentIndex());
				vlan->setSvlanId(leSvlanId->text().toULong(&isOk));
				vlan->setStpid(leSvlanTpid->text().remove(QChar(' ')).toULong(&isOk, 16));
				if (cbSvlanTpidOverride->isChecked())
					f |= VlanProtocol::VlanStpidOverride;
				if (gbSvlan->isChecked())
					f |= VlanProtocol::VlanSvlanTagged;

				vlan->setVlanFlags(f);
			}
		}
	}

	// L3
	{
		// L3 | IP
		{
			IpProtocol *ip = pStream->ip();
			IpProtocol::IpFlags f = 0;
			int ff = 0;

			ip->setVer(leIpVersion->text().toULong(&isOk));
			if (cbIpVersionOverride->isChecked())
				f |= IpProtocol::IpOverrideVersion;
			ip->setHdrLen(leIpHdrLen->text().toULong(&isOk));
			if (cbIpHdrLenOverride->isChecked())
				f |= IpProtocol::IpOverrideHdrLen;

			ip->setTos(leIpTos->text().toULong(&isOk, 16));

			ip->setTotLen(leIpLength->text().toULong(&isOk));
			if (cbIpLengthOverride->isChecked())
				f |= IpProtocol::IpOverrideHdrLen;

			ip->setId(leIpId->text().remove(QChar(' ')).toULong(&isOk, 16));
			ip->setFragOfs(leIpFragOfs->text().toULong(&isOk));

			if (cbIpFlagsDf->isChecked()) ff |= IP_FLAG_DF;
			if (cbIpFlagsMf->isChecked()) ff |= IP_FLAG_MF;
			ip->setFlags(ff);

			ip->setTtl(leIpTtl->text().toULong(&isOk));
			ip->setProto(leIpProto->text().remove(QChar(' ')).toULong(&isOk, 16));
			
			ip->setCksum(leIpCksum->text().remove(QChar(' ')).toULong(&isOk));
			if (cbIpCksumOverride->isChecked())
				f |= IpProtocol::IpOverrideCksum;

			ip->setSrcIp(QHostAddress(leIpSrcAddr->text()).toIPv4Address());
			ip->setSrcIpMode((IpProtocol::IpAddrMode) cmbIpSrcAddrMode->currentIndex());
			ip->setSrcIpCount(leIpSrcAddrCount->text().toULong(&isOk));
			ip->setSrcIpMask(QHostAddress(leIpSrcAddrMask->text()).toIPv4Address());

			ip->setDstIp(QHostAddress(leIpDstAddr->text()).toIPv4Address());
			ip->setDstIpMode((IpProtocol::IpAddrMode) cmbIpDstAddrMode->currentIndex());
			ip->setDstIpCount(leIpDstAddrCount->text().toULong(&isOk));
			ip->setDstIpMask(QHostAddress(leIpDstAddrMask->text()).toIPv4Address());

			ip->setIpFlags(f);
		}

		// L3 | ARP
		{
			// TODO(LOW)
		}
	}

	// L4
	{
		// L4 | TCP
		{
			TcpProtocol *tcp = pStream->tcp();
			TcpProtocol::TcpFlags f = 0;
			int ff = 0;

			tcp->setSrcPort(leTcpSrcPort->text().toULong(&isOk));
			tcp->setDstPort(leTcpDstPort->text().toULong(&isOk));

			tcp->setSeqNum(leTcpSeqNum->text().toULong(&isOk));
			tcp->setAckNum(leTcpAckNum->text().toULong(&isOk));

			tcp->setHdrLen(leTcpHdrLen->text().toULong(&isOk));
			if (cbTcpHdrLenOverride->isChecked())
				f |= TcpProtocol::TcpOverrideHdrLen;

			tcp->setWindow(leTcpWindow->text().toULong(&isOk));

			tcp->setCksum(leTcpCksum->text().remove(QChar(' ')).toULong(&isOk));
			if (cbTcpCksumOverride->isChecked())
				f |= TcpProtocol::TcpOverrideCksum;

			tcp->setUrgPtr(leTcpUrgentPointer->text().toULong(&isOk));

			if (cbTcpFlagsUrg->isChecked()) ff |= TCP_FLAG_URG;
			if (cbTcpFlagsAck->isChecked()) ff |= TCP_FLAG_ACK;
			if (cbTcpFlagsPsh->isChecked()) ff |= TCP_FLAG_PSH;
			if (cbTcpFlagsRst->isChecked()) ff |= TCP_FLAG_RST;
			if (cbTcpFlagsSyn->isChecked()) ff |= TCP_FLAG_SYN;
			if (cbTcpFlagsFin->isChecked()) ff |= TCP_FLAG_FIN;
			tcp->setFlags(ff);

			tcp->setTcpFlags(f);
		}

		// L4 | UDP
		{
			UdpProtocol *udp = pStream->udp();
			UdpProtocol::UdpFlags f = 0;

			udp->setSrcPort(leUdpSrcPort->text().toULong(&isOk));
			udp->setDstPort(leUdpDstPort->text().toULong(&isOk));

			udp->setTotLen(leUdpLength->text().toULong(&isOk));

			if (cbUdpLengthOverride->isChecked())
				f |= UdpProtocol::UdpOverrideTotLen;

			udp->setCksum(leUdpCksum->text().remove(QChar(' ')).toULong(&isOk));
			if (cbUdpCksumOverride->isChecked())
				f |= UdpProtocol::UdpOverrideCksum;

			udp->setUdpFlags(f);
		}

		// L4 | ICMP
		{
			// TODO)(LOW)
		}

		// L4 | IGMP
		{
			// TODO(LOW)
		}		
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
	StoreCurrentStream();
	qDebug("stream stored");
	lastTopLevelTabIndex = twTopLevel->currentIndex();
	lastProtoTabIndex = twProto->currentIndex();
}

//Junk Line for introducing a compiler error

