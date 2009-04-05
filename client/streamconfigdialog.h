#ifndef _STREAM_CONFIG_DIALOG_H
#define _STREAM_CONFIG_DIALOG_H

#include <QDialog>
#include "ui_streamconfigdialog.h"
#include "port.h"
#include "stream.h"
#include "packetmodel.h"
#include "modeltest.h"

#define MAX_MAC_ITER_COUNT	256
#define MIN_PKT_LEN			64
#define MAX_PKT_LEN			1522

/*
** TODO
** - Improve HexStr handling
**
*/


class StreamConfigDialog : public QDialog, public Ui::StreamConfigDialog
{
	Q_OBJECT
public:
	StreamConfigDialog(Port &port, uint streamIndex, QWidget *parent = 0);
	~StreamConfigDialog();

private: 
	//QList<Stream>	*mpStreamList;

	Port&			mPort;
	uint			mCurrentStreamIndex;
	PacketModel		*mpPacketModel;
	ModelTest		*mpPacketModelTester;

	// The following static variables are used to track the "selected" tab
    // for the various tab widgets so that it can be restored when the dialog
	// is opened the next time
	static int		lastTopLevelTabIndex;
	static int		lastProtoTabIndex;

	void setupUiExtra();
	void LoadCurrentStream();
	void StoreCurrentStream();

private slots:
	void on_cmbPatternMode_currentIndexChanged(QString mode);
	void on_cmbPktLenMode_currentIndexChanged(QString mode);
	void on_cmbDstMacMode_currentIndexChanged(QString mode);
	void on_cmbSrcMacMode_currentIndexChanged(QString mode);
	void on_cmbIpSrcAddrMode_currentIndexChanged(QString mode);
	void on_cmbIpDstAddrMode_currentIndexChanged(QString mode);
	void on_pbPrev_clicked();
	void on_pbNext_clicked();

	void on_rbFtNone_toggled(bool checked);
	void on_rbFtEthernet2_toggled(bool checked);
	void on_rbFt802Dot3Raw_toggled(bool checked);
	void on_rbFt802Dot3Llc_toggled(bool checked);
	void on_rbFtLlcSnap_toggled(bool checked);

	void on_rbL3Ipv4_toggled(bool checked);
	void on_rbL3Arp_toggled(bool checked);

	void on_rbL4Icmp_toggled(bool checked);
	void on_rbL4Igmp_toggled(bool checked);
	void on_rbL4Tcp_toggled(bool checked);
	void on_rbL4Udp_toggled(bool checked);

	void update_NumPacketsAndNumBursts();

	void on_pbOk_clicked();
};

QString & uintToHexStr(quint64 num, QString &hexStr, quint8 octets);

#endif

