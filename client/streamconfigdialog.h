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
** \todo Improve HexStr handling
**
*/


class StreamConfigDialog : public QDialog, public Ui::StreamConfigDialog
{
	Q_OBJECT
public:
	StreamConfigDialog(Port &port, uint streamIndex, QWidget *parent = 0);
	~StreamConfigDialog();

private: 

	QButtonGroup	*bgFrameType;
	QButtonGroup	*bgL3Proto;
	QButtonGroup	*bgL4Proto;

	QStringListModel *mpAvailableProtocolsModel;

	Port&			mPort;
	uint			mCurrentStreamIndex;

	Stream					*mpStream;
	ProtocolListIterator	*_iter;

	PacketModel		*mpPacketModel;
	ModelTest		*mpPacketModelTester;

	// The following static variables are used to track the "selected" tab
    // for the various tab widgets so that it can be restored when the dialog
	// is opened the next time
	static int		lastTopLevelTabIndex;
	static int		lastProtoTabIndex;

	void setupUiExtra();
	void updateSelectedProtocols();
	void LoadCurrentStream();
	void StoreCurrentStream();

private slots:
	void on_cmbPktLenMode_currentIndexChanged(QString mode);
	void on_pbPrev_clicked();
	void on_pbNext_clicked();

	void updateContents();

	void on_twTopLevel_currentChanged(int index);
	void on_twProto_currentChanged(int index);

	void update_NumPacketsAndNumBursts();

	void on_pbOk_clicked();
};

#endif

