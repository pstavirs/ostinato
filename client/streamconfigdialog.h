/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef _STREAM_CONFIG_DIALOG_H
#define _STREAM_CONFIG_DIALOG_H

#include <QDialog>
#include "ui_streamconfigdialog.h"
#include "port.h"
#include "stream.h"
#include "packetmodel.h"
#include "modeltest.h"

#define MAX_MAC_ITER_COUNT     256
#define MIN_PKT_LEN            64
#define MAX_PKT_LEN            16384

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

    enum ButtonId
    {
        ButtonIdNone = 0,
        ButtonIdOther = -2
    };

    enum ProtoButtonGroup
    {
        ProtoMin,
        ProtoL1 = 0,
        ProtoVlan = 1,
        ProtoL2 = 2,
        ProtoL3 = 3,
        ProtoL4 = 4,
        ProtoL5 = 5,
        ProtoPayload = 6,
        ProtoMax
    };

    QButtonGroup    *bgProto[ProtoMax];

    QStringListModel *mpAvailableProtocolsModel;
    QStringListModel *mpSelectedProtocolsModel;

    Port&            mPort;
    uint            mCurrentStreamIndex;

    Stream                    *mpStream;
    ProtocolListIterator    *_iter;

    bool            isUpdateInProgress;

    PacketModel        *mpPacketModel;
    ModelTest        *mpPacketModelTester;

    // The following static variables are used to track the "selected" tab
    // for the various tab widgets so that it can be restored when the dialog
    // is opened the next time. We also track the last Dialog geometry.
    static QRect lastGeometry;
    static int lastTopLevelTabIndex;
    static int lastProtocolDataIndex;

    void setupUiExtra();
    void LoadCurrentStream();
    void StoreCurrentStream();

private slots:
    void on_cmbPktLenMode_currentIndexChanged(QString mode);
    void update_NumPacketsAndNumBursts();

    void on_twTopLevel_currentChanged(int index);
    void on_tbSelectProtocols_currentChanged(int index);

    // "Simple" Protocol Selection related
    bool skipProtocols(int layer);

    void disableProtocols(QButtonGroup *protocolGroup, bool checked);
    void forceProtocolNone(bool checked);

    void updateProtocol(int newId);
    void __updateProtocol(int level, int newId);

    void updateSelectProtocolsSimpleWidget();

    // "Advanced" Protocol Selection related
    void when_lvAllProtocols_selectionChanged(
        const QItemSelection &selected, const QItemSelection &deselected);
    void when_lvSelectedProtocols_currentChanged(
        const QModelIndex &current, const QModelIndex &previous);

    void on_tbAdd_clicked();
    void on_tbDelete_clicked();
    void on_tbUp_clicked();
    void on_tbDown_clicked();

    void updateSelectProtocolsAdvancedWidget();

    // "Protocol Data" related
    void on_tbProtocolData_currentChanged(int index);

    // "Stream Control" related
    void on_rbPacketsPerSec_toggled(bool checked);
    void on_rbBurstsPerSec_toggled(bool checked);

    void on_lePacketsPerBurst_textChanged(const QString &text);
    void on_lePacketsPerSec_textChanged(const QString &text);
    void on_leBurstsPerSec_textChanged(const QString &text);
    void on_leBitsPerSec_textEdited(const QString &text);

    void on_pbPrev_clicked();
    void on_pbNext_clicked();

    void on_pbOk_clicked();
};

#endif

