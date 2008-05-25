#include <QHostAddress>
#include "streamconfigdialog.h"
#include "stream.h"

// TODO(LOW): Remove
#include "modeltest.h"

StreamConfigDialog::StreamConfigDialog(QList<Stream> *streamList,
	uint streamIndex, QWidget *parent) : QDialog (parent)
{
	setupUi(this);
	setupUiExtra();
	
	// FIXME(MED): Assumption that streamlist and streamIndex are valid
	mpStreamList = streamList;
	mCurrentStreamIndex = streamIndex;
	LoadCurrentStream();

	mpPacketModel = new PacketModel(&((*mpStreamList)[mCurrentStreamIndex]),
		this);
	tvPacketTree->setModel(mpPacketModel);
	mpPacketModelTester = new ModelTest(mpPacketModel);
	tvPacketTree->header()->hide();

	qDebug("stream %p %d/%d loaded", 
		mpStreamList, mCurrentStreamIndex, mpStreamList->size());

// FIXME(MED): Enable this navigation
#if 0
	pbPrev->setDisabled((currStreamIdx == 0));
	pbNext->setDisabled((currStreamIdx == 2));
#endif
}

void StreamConfigDialog::setupUiExtra()
{
	QRegExp reHex2B("[0-9,a-f,A-F]{1,4}");
	QRegExp reHex4B("[0-9,a-f,A-F]{1,8}");
	QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

	// Setup default stuff that cannot be done in designer
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

	// Show "Packet Config" page by default
	twTopLevel->setCurrentIndex(0);
}

StreamConfigDialog::~StreamConfigDialog()
{
	delete mpPacketModelTester;
	delete mpPacketModel;
}

void StreamConfigDialog::on_cmbDstMacMode_currentIndexChanged(QString mode)
{
	if (mode == "Fixed")
		leDstMacCount->setEnabled(FALSE);
	else
		leDstMacCount->setEnabled(TRUE);
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

void StreamConfigDialog::on_rbL4Other_toggled(bool checked)
{
	if (checked)
		leIpProto->setEnabled(true);
	else
		leIpProto->setEnabled(false);
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

QString & uintToHexStr(quint32 num, QString &hexStr, quint8 octets) 
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
	Stream	*pStream = &((*mpStreamList)[mCurrentStreamIndex]);
	QString	str;

	qDebug("loading pStream %p", pStream);

	// Meta Data
	{
		cmbPatternMode->setCurrentIndex(pStream->meta.patternMode);
		lePattern->setText(uintToHexStr(pStream->meta.pattern, str, 4));

		cmbPktLenMode->setCurrentIndex(pStream->meta.lenMode);
		lePktLen->setText(str.setNum(pStream->meta.frameLen));
		lePktLenMin->setText(str.setNum(pStream->meta.frameLenMin));
		lePktLenMax->setText(str.setNum(pStream->meta.frameLenMax));
	}

	// Protocols
	{
		qDebug("ft = %d\n", pStream->proto.ft);
		switch(pStream->proto.ft)
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
		leDsap->setText(uintToHexStr(pStream->proto.dsap, str, 1));
		leSsap->setText(uintToHexStr(pStream->proto.ssap, str, 1));
		leControl->setText(uintToHexStr(pStream->proto.ctl, str, 1));
		leOui->setText(uintToHexStr((pStream->proto.ouiMsb << 16 + pStream->proto.ouiLshw), str, 3));

		leType->setText(uintToHexStr(pStream->proto.etherType, str, 2));

		// Check for specific supported protocols first ...
		if (pStream->proto.etherType == ETH_TYP_IP)
			rbL3Ipv4->setChecked(TRUE);
		else if (pStream->proto.etherType == ETH_TYP_ARP)
			rbL3Arp->setChecked(TRUE);

		// ... then for None/Other
		rbL3None->setChecked((pStream->proto.protoMask & PM_L3_PROTO_NONE) > 0);
		rbL3Other->setChecked((pStream->proto.protoMask & PM_L3_PROTO_OTHER) > 0);

		// Check for specific supported protocols first ...
		if (pStream->proto.ipProto == IP_PROTO_ICMP)
			rbL4Icmp->setChecked(TRUE);
		else if (pStream->proto.ipProto == IP_PROTO_IGMP)
			rbL4Igmp->setChecked(TRUE);
		else if (pStream->proto.ipProto == IP_PROTO_TCP)
			rbL4Tcp->setChecked(TRUE);
		else if (pStream->proto.ipProto == IP_PROTO_UDP)
			rbL4Udp->setChecked(TRUE);

		// ... then for None/Other
		rbL4None->setChecked((pStream->proto.protoMask & PM_L4_PROTO_NONE) > 0);
		rbL4Other->setChecked((pStream->proto.protoMask & PM_L4_PROTO_OTHER) > 0);
	}

	// L2
	{
		// L2 | Ethernet
		{
			leDstMac->setText(uintToHexStr(pStream->l2.eth.dstMacMshw, str, 2) + 
					uintToHexStr(pStream->l2.eth.dstMacLsw, str, 4));
			cmbDstMacMode->setCurrentIndex(pStream->l2.eth.dstMacMode);
			leDstMacCount->setText(str.setNum(pStream->l2.eth.dstMacCount));
			leDstMacStep->setText(str.setNum(pStream->l2.eth.dstMacStep));

			leSrcMac->setText(uintToHexStr(pStream->l2.eth.srcMacMshw, str, 2) + 
					uintToHexStr(pStream->l2.eth.srcMacLsw, str, 4));
			cmbSrcMacMode->setCurrentIndex(pStream->l2.eth.srcMacMode);
			leSrcMacCount->setText(str.setNum(pStream->l2.eth.srcMacCount));
			leSrcMacStep->setText(str.setNum(pStream->l2.eth.srcMacStep));
			
			cmbCvlanPrio->setCurrentIndex(pStream->l2.eth.cvlanPrio);
			cmbCvlanCfi->setCurrentIndex(pStream->l2.eth.cvlanCfi);
			leCvlanId->setText(str.setNum(pStream->l2.eth.cvlanId));
			leCvlanTpid->setText(str.setNum(pStream->l2.eth.ctpid));
			cbCvlanTpidOverride->setChecked((pStream->l2.eth.vlanMask & VM_CVLAN_TPID_OVERRIDE) > 0);
			gbCvlan->setChecked((pStream->l2.eth.vlanMask & VM_CVLAN_TAGGED) > 0);

			cmbSvlanPrio->setCurrentIndex(pStream->l2.eth.svlanPrio);
			cmbSvlanCfi->setCurrentIndex(pStream->l2.eth.svlanCfi);
			leSvlanId->setText(str.setNum(pStream->l2.eth.svlanId));
			leSvlanTpid->setText(str.setNum(pStream->l2.eth.stpid));
			cbSvlanTpidOverride->setChecked((pStream->l2.eth.vlanMask & VM_SVLAN_TPID_OVERRIDE) > 0);
			gbSvlan->setChecked((pStream->l2.eth.vlanMask & VM_SVLAN_TAGGED) > 0);
		}
	}
			
	// L3
	{
		// L3 | IP
		{
			leIpVersion->setText(str.setNum(pStream->l3.ip.ver));
			cbIpVersionOverride->setChecked((pStream->l3.ip.ipMask & IM_OVERRIDE_VERSION) > 0);
			leIpHdrLen->setText(str.setNum(pStream->l3.ip.hdrLen));
			cbIpHdrLenOverride->setChecked((pStream->l3.ip.ipMask & IM_OVERRIDE_HDRLEN) > 0);
			
			leIpTos->setText(uintToHexStr(pStream->l3.ip.tos, str, 1));

			leIpLength->setText(str.setNum(pStream->l3.ip.totLen));
			cbIpLengthOverride->setChecked((pStream->l3.ip.ipMask & IM_OVERRIDE_TOTLEN) > 0);

			leIpId->setText(uintToHexStr(pStream->l3.ip.id, str, 2));
			leIpFragOfs->setText(str.setNum(pStream->l3.ip.fragOfs));
			cbIpFlagsDf->setChecked((pStream->l3.ip.flags & IP_FLAG_DF) > 0);
			cbIpFlagsMf->setChecked((pStream->l3.ip.flags & IP_FLAG_MF) > 0);

			leIpTtl->setText(str.setNum(pStream->l3.ip.ttl));
			leIpProto->setText(uintToHexStr(pStream->l3.ip.proto, str, 1));

			leIpCksum->setText(uintToHexStr(pStream->l3.ip.cksum, str, 2));
			cbIpCksumOverride->setChecked((pStream->l3.ip.ipMask & IM_OVERRIDE_CKSUM) > 0);

			leIpSrcAddr->setText(QHostAddress(pStream->l3.ip.srcIp).toString());
			cmbIpSrcAddrMode->setCurrentIndex(pStream->l3.ip.srcIpMode);
			leIpSrcAddrCount->setText(str.setNum(pStream->l3.ip.srcIpCount));
			leIpSrcAddrMask->setText(QHostAddress(pStream->l3.ip.srcIpMask).toString());

			leIpDstAddr->setText(QHostAddress(pStream->l3.ip.dstIp).toString());
			cmbIpDstAddrMode->setCurrentIndex(pStream->l3.ip.dstIpMode);
			leIpDstAddrCount->setText(str.setNum(pStream->l3.ip.dstIpCount));
			leIpDstAddrMask->setText(QHostAddress(pStream->l3.ip.dstIpMask).toString());
		}

		// L3 | ARP
		{
			// TODO
		}
	}

	// L4
	{
		// L4 | TCP
		{
			leTcpSrcPort->setText(str.setNum(pStream->l4.tcp.srcPort));
			leTcpDstPort->setText(str.setNum(pStream->l4.tcp.dstPort));

			leTcpSeqNum->setText(str.setNum(pStream->l4.tcp.seqNum));
			leTcpAckNum->setText(str.setNum(pStream->l4.tcp.ackNum));

			leTcpHdrLen->setText(str.setNum(pStream->l4.tcp.hdrLen));
			cbTcpHdrLenOverride->setChecked((pStream->l4.tcp.tcpMask & TM_OVERRIDE_HDRLEN) > 0);

			leTcpWindow->setText(str.setNum(pStream->l4.tcp.window));

			leTcpCksum->setText(str.setNum(pStream->l4.tcp.cksum));
			cbTcpCksumOverride->setChecked((pStream->l4.tcp.tcpMask & TM_OVERRIDE_CKSUM) > 0);

			leTcpUrgentPointer->setText(str.setNum(pStream->l4.tcp.urgPtr));

			cbTcpFlagsUrg->setChecked((pStream->l4.tcp.flags & TCP_FLAG_URG) > 0);
			cbTcpFlagsAck->setChecked((pStream->l4.tcp.flags & TCP_FLAG_ACK) > 0);
			cbTcpFlagsPsh->setChecked((pStream->l4.tcp.flags & TCP_FLAG_PSH) > 0);
			cbTcpFlagsRst->setChecked((pStream->l4.tcp.flags & TCP_FLAG_RST) > 0);
			cbTcpFlagsSyn->setChecked((pStream->l4.tcp.flags & TCP_FLAG_SYN) > 0);
			cbTcpFlagsFin->setChecked((pStream->l4.tcp.flags & TCP_FLAG_FIN) > 0);
		}

		// L4 | UDP
		{
			leUdpSrcPort->setText(str.setNum(pStream->l4.udp.srcPort));
			leUdpDstPort->setText(str.setNum(pStream->l4.udp.dstPort));

			leUdpLength->setText(str.setNum(pStream->l4.udp.totLen));
			cbUdpLengthOverride->setChecked((pStream->l4.udp.udpMask & UM_OVERRIDE_TOTLEN) > 0);

			leUdpCksum->setText(str.setNum(pStream->l4.udp.cksum));
			cbUdpCksumOverride->setChecked((pStream->l4.udp.udpMask & UM_OVERRIDE_CKSUM) > 0);
		}

		// L4 | ICMP
		{
			// TODO
		}

		// L4 | IGMP
		{
			// TODO
		}
	}
}

void StreamConfigDialog::StoreCurrentStream()
{
	Stream	*pStream = &(*mpStreamList)[mCurrentStreamIndex];
	QString	str;
	bool	isOk;

	qDebug("storing pStream %p", pStream);

	// Meta Data
	pStream->meta.patternMode = (Stream::DataPatternMode) cmbPatternMode->currentIndex();
	pStream->meta.pattern = lePattern->text().remove(QChar(' ')).toULong(&isOk, 16);

	pStream->meta.lenMode = (Stream::FrameLengthMode) cmbPktLenMode->currentIndex();
	pStream->meta.frameLen = lePktLen->text().toULong(&isOk);
	pStream->meta.frameLenMin = lePktLenMin->text().toULong(&isOk);
	pStream->meta.frameLenMax = lePktLenMax->text().toULong(&isOk);

	// Protocols
	{
		if (rbFtNone->isChecked()) 
			pStream->proto.ft = Stream::e_ft_none;
		else if (rbFtEthernet2->isChecked())
			pStream->proto.ft = Stream::e_ft_eth_2;
		else if (rbFt802Dot3Raw->isChecked())
			pStream->proto.ft = Stream::e_ft_802_3_raw;
		else if (rbFt802Dot3Llc->isChecked())
			pStream->proto.ft = Stream::e_ft_802_3_llc;
		else if (rbFtLlcSnap->isChecked())
			pStream->proto.ft = Stream::e_ft_snap;

		qDebug("store ft = %d\n", pStream->proto.ft);

		pStream->proto.dsap = leDsap->text().remove(QChar(' ')).toULong(&isOk, 16);
		pStream->proto.ssap = leSsap->text().remove(QChar(' ')).toULong(&isOk, 16);
		pStream->proto.ctl = leControl->text().remove(QChar(' ')).toULong(&isOk, 16);
		pStream->proto.ouiMsb = leOui->text().remove(QChar(' ')).toULong(&isOk, 16) >> 16;
		pStream->proto.ouiLshw = 0x0000FFFF & leOui->text().remove(QChar(' ')).toULong(&isOk, 16);
		pStream->proto.etherType = leType->text().remove(QChar(' ')).toULong(&isOk, 16);

		pStream->proto.ipProto = leIpProto->text().remove(QChar(' ')).toULong(&isOk, 16);

		// Just check for None/Other - no need to do anything for specific supported protocols
		pStream->proto.protoMask = 0;
		if (rbL3None->isChecked())
			pStream->proto.protoMask |= PM_L3_PROTO_NONE;
		else if (rbL3Other->isChecked())
			pStream->proto.protoMask |= PM_L3_PROTO_OTHER;

		if (rbL4None->isChecked())
			pStream->proto.protoMask |= PM_L4_PROTO_NONE;
		else if (rbL4Other->isChecked())
			pStream->proto.protoMask |= PM_L4_PROTO_OTHER;
	}

	// L2
	{
		// L2 | Ethernet
		{
			pStream->l2.eth.dstMacMshw = leDstMac->text().remove(QChar(' ')).left(4).toULong(&isOk, 16);
			pStream->l2.eth.dstMacLsw = leDstMac->text().remove(QChar(' ')).right(8).toULong(&isOk, 16);
			pStream->l2.eth.dstMacMode = (Stream::MacAddrMode) cmbDstMacMode->currentIndex();
			pStream->l2.eth.dstMacCount = leDstMacCount->text().toULong(&isOk);
			pStream->l2.eth.dstMacStep = leDstMacStep->text().toULong(&isOk);

			pStream->l2.eth.srcMacMshw = leSrcMac->text().remove(QChar(' ')).left(4).toULong(&isOk, 16);
			pStream->l2.eth.srcMacLsw = leSrcMac->text().remove(QChar(' ')).right(8).toULong(&isOk, 16);
			pStream->l2.eth.srcMacMode = (Stream::MacAddrMode) cmbSrcMacMode->currentIndex();
			pStream->l2.eth.srcMacCount = leSrcMacCount->text().toULong(&isOk);
			pStream->l2.eth.srcMacStep = leSrcMacStep->text().toULong(&isOk);

			pStream->l2.eth.vlanMask = 0;

			pStream->l2.eth.cvlanPrio = cmbCvlanPrio->currentIndex();
			pStream->l2.eth.cvlanCfi = cmbCvlanCfi->currentIndex();
			pStream->l2.eth.cvlanId = leCvlanId->text().toULong(&isOk);
			pStream->l2.eth.ctpid = leCvlanTpid->text().remove(QChar(' ')).toULong(&isOk);
			if (cbCvlanTpidOverride->isChecked())
				pStream->l2.eth.vlanMask |= VM_CVLAN_TPID_OVERRIDE;
			if (gbCvlan->isChecked())
				pStream->l2.eth.vlanMask |= VM_CVLAN_TAGGED;

			pStream->l2.eth.svlanPrio = cmbSvlanPrio->currentIndex();
			pStream->l2.eth.svlanCfi = cmbSvlanCfi->currentIndex();
			pStream->l2.eth.svlanId = leSvlanId->text().toULong(&isOk);
			pStream->l2.eth.stpid = leSvlanTpid->text().remove(QChar(' ')).toULong(&isOk);
			if (cbSvlanTpidOverride->isChecked())
				pStream->l2.eth.vlanMask |= VM_SVLAN_TPID_OVERRIDE;
			if (gbSvlan->isChecked())
				pStream->l2.eth.vlanMask |= VM_SVLAN_TAGGED;
		}
	}

	// L3
	{
		// L3 | IP
		{
			pStream->l3.ip.ipMask = 0;

			pStream->l3.ip.ver = leIpVersion->text().toULong(&isOk);
			if (cbIpVersionOverride->isChecked())
				pStream->l3.ip.ipMask |= IM_OVERRIDE_VERSION;
			pStream->l3.ip.hdrLen = leIpHdrLen->text().toULong(&isOk);
			if (cbIpHdrLenOverride->isChecked())
				pStream->l3.ip.ipMask |= IM_OVERRIDE_HDRLEN;

			pStream->l3.ip.tos = leIpTos->text().toULong(&isOk, 16);

			pStream->l3.ip.totLen = leIpLength->text().toULong(&isOk);
			if (cbIpLengthOverride->isChecked())
				pStream->l3.ip.ipMask |= IM_OVERRIDE_TOTLEN;

			pStream->l3.ip.id = leIpId->text().remove(QChar(' ')).toULong(&isOk, 16);
			pStream->l3.ip.fragOfs = leIpFragOfs->text().toULong(&isOk);
			if (cbIpFlagsDf->isChecked()) pStream->l3.ip.ipMask |= IP_FLAG_DF;
			if (cbIpFlagsMf->isChecked()) pStream->l3.ip.ipMask |= IP_FLAG_MF;

			pStream->l3.ip.ttl = leIpTtl->text().toULong(&isOk);
			pStream->l3.ip.proto = leIpProto->text().remove(QChar(' ')).toULong(&isOk, 16);
			
			pStream->l3.ip.cksum = leIpCksum->text().remove(QChar(' ')).toULong(&isOk);
			if (cbIpCksumOverride->isChecked())
				pStream->l3.ip.ipMask |= IM_OVERRIDE_CKSUM;

			pStream->l3.ip.srcIp = QHostAddress(leIpSrcAddr->text()).toIPv4Address();
			pStream->l3.ip.srcIpMode = (Stream::IpAddrMode) cmbIpSrcAddrMode->currentIndex();
			pStream->l3.ip.srcIpCount = leIpSrcAddrCount->text().toULong(&isOk);
			pStream->l3.ip.srcIpMask = QHostAddress(leIpSrcAddrMask->text()).toIPv4Address();

			pStream->l3.ip.dstIp = QHostAddress(leIpDstAddr->text()).toIPv4Address();
			pStream->l3.ip.dstIpMode = (Stream::IpAddrMode) cmbIpDstAddrMode->currentIndex();
			pStream->l3.ip.dstIpCount = leIpDstAddrCount->text().toULong(&isOk);
			pStream->l3.ip.dstIpMask = QHostAddress(leIpDstAddrMask->text()).toIPv4Address();
		}

		// L3 | ARP
		{
			// TODO
		}
	}

	// L4
	{
		// L4 | TCP
		{
			pStream->l4.tcp.tcpMask = 0;

			pStream->l4.tcp.srcPort = leTcpSrcPort->text().toULong(&isOk);
			pStream->l4.tcp.dstPort = leTcpDstPort->text().toULong(&isOk);

			pStream->l4.tcp.seqNum = leTcpSeqNum->text().toULong(&isOk);
			pStream->l4.tcp.ackNum = leTcpAckNum->text().toULong(&isOk);

			pStream->l4.tcp.hdrLen = leTcpHdrLen->text().toULong(&isOk);
			if (cbTcpHdrLenOverride->isChecked())
				pStream->l4.tcp.tcpMask |= TM_OVERRIDE_HDRLEN;

			pStream->l4.tcp.window = leTcpWindow->text().toULong(&isOk);

			pStream->l4.tcp.cksum = leTcpCksum->text().remove(QChar(' ')).toULong(&isOk);
			if (cbTcpCksumOverride->isChecked())
				pStream->l4.tcp.tcpMask |= TM_OVERRIDE_CKSUM;

			pStream->l4.tcp.urgPtr = leTcpUrgentPointer->text().toULong(&isOk);

			pStream->l4.tcp.flags = 0;
			if (cbTcpFlagsUrg->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_URG;
			if (cbTcpFlagsAck->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_ACK;
			if (cbTcpFlagsPsh->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_PSH;
			if (cbTcpFlagsRst->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_RST;
			if (cbTcpFlagsSyn->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_SYN;
			if (cbTcpFlagsFin->isChecked()) pStream->l4.tcp.flags |= TCP_FLAG_FIN;
		}

		// L4 | UDP
		{
			pStream->l4.udp.udpMask = 0;

			pStream->l4.udp.srcPort = leUdpSrcPort->text().toULong(&isOk);
			pStream->l4.udp.dstPort = leUdpDstPort->text().toULong(&isOk);

			pStream->l4.udp.totLen = leUdpLength->text().toULong(&isOk);
			if (cbUdpLengthOverride->isChecked()) pStream->l4.udp.udpMask |= UM_OVERRIDE_TOTLEN;

			pStream->l4.udp.cksum = leUdpCksum->text().remove(QChar(' ')).toULong(&isOk);
			if (cbUdpCksumOverride->isChecked()) pStream->l4.udp.udpMask |= UM_OVERRIDE_CKSUM;
		}

		// L4 | ICMP
		{
			// TODO
		}

		// L4 | IGMP
		{
			// TODO
		}		
	}
}
void StreamConfigDialog::on_pbOk_clicked()
{
	// Store dialog contents into stream
	StoreCurrentStream();
	qDebug("stream stored");
}

//Junk Line for introducing a compiler error

