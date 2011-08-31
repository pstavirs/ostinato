/*
Copyright (C) 2010-2011 Srivats P.

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

#include <QHostAddress>

#include "streamconfigdialog.h"
#include "stream.h"
#include "abstractprotocol.h"
#include "protocollistiterator.h"

#include "modeltest.h"

// FIXME(HI) - remove
#include "../common/protocolmanager.h"
extern ProtocolManager *OstProtocolManager;

QRect StreamConfigDialog::lastGeometry;
int StreamConfigDialog::lastTopLevelTabIndex = 0;
int StreamConfigDialog::lastProtocolDataIndex = 0;

static const uint kEthFrameOverHead = 20;

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

    for (int i = ProtoMin; i < ProtoMax; i++)
    {
        bgProto[i]->setProperty("ProtocolLevel", i);
        bgProto[i]->setProperty("ProtocolId", ButtonIdNone);
        connect(bgProto[i], SIGNAL(buttonClicked(int)),
            this, SLOT(updateProtocol(int)));
    }

    //! \todo causes a crash!
#if 0    
    connect(lePktLen, SIGNAL(textEdited(QString)),
        this, SLOT(updateContents()));
#endif

    // Time to play match the signals and slots!

    // If L1/L2(FT)/L3 = None, force subsequent protocol level(s) also to None
    connect(rbL1None, SIGNAL(toggled(bool)), SLOT(forceProtocolNone(bool)));
    connect(rbFtNone, SIGNAL(toggled(bool)), SLOT(forceProtocolNone(bool)));
    connect(rbL3None, SIGNAL(toggled(bool)), SLOT(forceProtocolNone(bool)));
    connect(rbL4None, SIGNAL(toggled(bool)), SLOT(forceProtocolNone(bool)));

    // If L1/L2(FT)/L3/L4 = Other, force subsequent protocol to Other and 
    // disable the subsequent protocol group as well
    connect(rbL1Other, SIGNAL(toggled(bool)), rbFtOther, SLOT(setChecked(bool)));
    connect(rbL1Other, SIGNAL(toggled(bool)), gbFrameType, SLOT(setDisabled(bool)));
    connect(rbFtOther, SIGNAL(toggled(bool)), rbL3Other, SLOT(setChecked(bool)));
    connect(rbFtOther, SIGNAL(toggled(bool)), gbL3Proto, SLOT(setDisabled(bool)));
    connect(rbL3Other, SIGNAL(toggled(bool)), rbL4Other, SLOT(setChecked(bool)));
    connect(rbL3Other, SIGNAL(toggled(bool)), gbL4Proto, SLOT(setDisabled(bool)));
    connect(rbL4Other, SIGNAL(toggled(bool)), rbPayloadOther, SLOT(setChecked(bool)));
    connect(rbL4Other, SIGNAL(toggled(bool)), gbPayloadProto, SLOT(setDisabled(bool)));

    // Setup valid subsequent protocols for L2 to L4 protocols
    for (int i = ProtoL2; i <= ProtoL4; i++)
    {
        foreach(QAbstractButton *btn1, bgProto[i]->buttons())
        {
            int id1 = bgProto[i]->id(btn1);

            if (id1 != ButtonIdNone && id1 != ButtonIdOther)
            {
                int validProtocolCount = 0;

                foreach(QAbstractButton *btn2, bgProto[i+1]->buttons())
                {
                    int id2 = bgProto[i+1]->id(btn2);

                    if (id2 != ButtonIdNone && id2 != ButtonIdOther)
                    {
                        if (OstProtocolManager->isValidNeighbour(id1, id2))
                        {
                            connect(btn1, SIGNAL(toggled(bool)), 
                                    btn2, SLOT(setEnabled(bool)));
                            validProtocolCount++;
                        }
                        else
                            connect(btn1, SIGNAL(toggled(bool)), 
                                    btn2, SLOT(setDisabled(bool)));
                    }
                }

                // If btn1 has no subsequent valid protocols, 
                // force subsequent Protocol to 'None'
                if (validProtocolCount == 0)
                    connect(btn1, SIGNAL(clicked(bool)),
                        bgProto[i+1]->button(ButtonIdNone), SLOT(click()));

                // If the id1 protocol doesn't have a payload (e.g. IGMP)
                // force payload protocol to 'None'
                if (!OstProtocolManager->protocolHasPayload(id1))
                {
                    connect(btn1, SIGNAL(clicked(bool)),
                        bgProto[ProtoPayload]->button(ButtonIdNone), 
                        SLOT(click()));
                }
            }
        }
    }

    mpAvailableProtocolsModel = new QStringListModel(
        OstProtocolManager->protocolDatabase(), this);
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
#ifdef QT_NO_DEBUG
    mpPacketModelTester = NULL;
#else
    mpPacketModelTester = new ModelTest(mpPacketModel);
#endif
    tvPacketTree->header()->hide();
    vwPacketDump->setModel(mpPacketModel);
    vwPacketDump->setSelectionModel(tvPacketTree->selectionModel());

    // TODO(MED):
    //! \todo Enable navigation of streams
    pbPrev->setHidden(true);
    pbNext->setHidden(true);
    //! \todo Support Goto Stream Id
    leStreamId->setHidden(true);
    disconnect(rbActionGotoStream, SIGNAL(toggled(bool)), leStreamId, SLOT(setEnabled(bool)));

    switch(mPort.transmitMode())
    {
    case OstProto::kSequentialTransmit:
        rbModeFixed->setChecked(true);
        rbModeContinuous->setDisabled(true);
        break;
    case OstProto::kInterleavedTransmit:
        rbModeContinuous->setChecked(true);
        rbModeFixed->setDisabled(true);

        nextWhat->setHidden(true);
        break;
    default:
        Q_ASSERT(false); // Unreachable
    }

    // Finally, restore the saved last geometry and selected tab for the 
    // various tab widgets
    if (!lastGeometry.isNull())
        setGeometry(lastGeometry);
    twTopLevel->setCurrentIndex(lastTopLevelTabIndex);
}

void StreamConfigDialog::setupUiExtra()
{
    QRegExp reHex2B("[0-9,a-f,A-F]{1,4}");
    QRegExp reHex4B("[0-9,a-f,A-F]{1,8}");
    QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

    // ---- Setup default stuff that cannot be done in designer ----
    bgProto[ProtoL1] = new QButtonGroup();
    bgProto[ProtoL1]->addButton(rbL1None, ButtonIdNone);
    bgProto[ProtoL1]->addButton(rbL1Mac, OstProto::Protocol::kMacFieldNumber);
    bgProto[ProtoL1]->addButton(rbL1Other, ButtonIdOther);

    bgProto[ProtoL2] = new QButtonGroup();
#if 0
    foreach(QRadioButton *btn, gbFrameType->findChildren<QRadioButton*>())
        bgL2Proto->addButton(btn);
#else
    bgProto[ProtoL2]->addButton(rbFtNone, ButtonIdNone);
    bgProto[ProtoL2]->addButton(rbFtEthernet2, OstProto::Protocol::kEth2FieldNumber);
    bgProto[ProtoL2]->addButton(rbFt802Dot3Raw, OstProto::Protocol::kDot3FieldNumber);
    bgProto[ProtoL2]->addButton(rbFt802Dot3Llc, OstProto::Protocol::kDot2LlcFieldNumber);
    bgProto[ProtoL2]->addButton(rbFtLlcSnap, OstProto::Protocol::kDot2SnapFieldNumber);
    bgProto[ProtoL2]->addButton(rbFtOther, ButtonIdOther);
#endif

    bgProto[ProtoVlan] = new QButtonGroup();
    bgProto[ProtoVlan]->addButton(rbVlanNone, ButtonIdNone);
    bgProto[ProtoVlan]->addButton(rbVlanSingle, OstProto::Protocol::kVlanFieldNumber);
    bgProto[ProtoVlan]->addButton(rbVlanDouble, OstProto::Protocol::kVlanStackFieldNumber);

    bgProto[ProtoL3] = new QButtonGroup();
#if 0
    foreach(QRadioButton *btn, gbL3Proto->findChildren<QRadioButton*>())
        bgProto[ProtoL3]->addButton(btn);
#else
    bgProto[ProtoL3]->addButton(rbL3None, ButtonIdNone);
    bgProto[ProtoL3]->addButton(rbL3Arp, OstProto::Protocol::kArpFieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ipv4, OstProto::Protocol::kIp4FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ipv6, OstProto::Protocol::kIp6FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ip6over4, 
            OstProto::Protocol::kIp6over4FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ip4over6, 
            OstProto::Protocol::kIp4over6FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ip4over4, 
            OstProto::Protocol::kIp4over4FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Ip6over6, 
            OstProto::Protocol::kIp6over6FieldNumber);
    bgProto[ProtoL3]->addButton(rbL3Other, ButtonIdOther);
#endif

    bgProto[ProtoL4] = new QButtonGroup();
#if 0
    foreach(QRadioButton *btn, gbL4Proto->findChildren<QRadioButton*>())
        bgProto[ProtoL4]->addButton(btn);
#else
    bgProto[ProtoL4]->addButton(rbL4None, ButtonIdNone);
    bgProto[ProtoL4]->addButton(rbL4Tcp, OstProto::Protocol::kTcpFieldNumber);
    bgProto[ProtoL4]->addButton(rbL4Udp, OstProto::Protocol::kUdpFieldNumber);
    bgProto[ProtoL4]->addButton(rbL4Icmp, OstProto::Protocol::kIcmpFieldNumber);
    bgProto[ProtoL4]->addButton(rbL4Igmp, OstProto::Protocol::kIgmpFieldNumber);
    bgProto[ProtoL4]->addButton(rbL4Mld, OstProto::Protocol::kMldFieldNumber);
    bgProto[ProtoL4]->addButton(rbL4Other, ButtonIdOther);
#endif

    bgProto[ProtoL5] = new QButtonGroup();
#if 0
    foreach(QRadioButton *btn, gbL5Proto->findChildren<QRadioButton*>())
        bgProto[ProtoL5]->addButton(btn);
#else
    bgProto[ProtoL5]->addButton(rbL5None, ButtonIdNone);
    bgProto[ProtoL5]->addButton(rbL5Text, 
                                OstProto::Protocol::kTextProtocolFieldNumber);
    bgProto[ProtoL5]->addButton(rbL5Other, ButtonIdOther);
#endif

    bgProto[ProtoPayload] = new QButtonGroup();
#if 0
    foreach(QRadioButton *btn, gbPayloadProto->findChildren<QRadioButton*>())
        bgProto[ProtoPayload]->addButton(btn);
#else
    bgProto[ProtoPayload]->addButton(rbPayloadNone, ButtonIdNone);
    bgProto[ProtoPayload]->addButton(rbPayloadPattern, OstProto::Protocol::kPayloadFieldNumber);
    bgProto[ProtoPayload]->addButton(rbPayloadHexDump, OstProto::Protocol::kHexDumpFieldNumber);
    bgProto[ProtoPayload]->addButton(rbPayloadOther, ButtonIdOther);
#endif
    /*
    ** Setup Validators
    */    
    // Meta Data
    lePktLen->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN, this));
    lePktLenMin->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN,this));
    lePktLenMax->setValidator(new QIntValidator(MIN_PKT_LEN, MAX_PKT_LEN,this));

    lePacketsPerBurst->setValidator(new QIntValidator(1, 0x7FFFFFFF,this));

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

    for (int i = ProtoMin; i < ProtoMax; i++)
        delete bgProto[i];

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
    const QItemSelection &/*selected*/, const QItemSelection &/*deselected*/)
{
    int size = lvAllProtocols->selectionModel()->selectedIndexes().size();

    qDebug("%s: selected.indexes().size = %d\n", __FUNCTION__, size);

    tbAdd->setEnabled(size > 0);
}

void StreamConfigDialog::when_lvSelectedProtocols_currentChanged(
    const QModelIndex &current, const QModelIndex &/*previous*/)
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
    QModelIndexList    selection;

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
        _iter->insert(OstProtocolManager->createProtocol(
            mpAvailableProtocolsModel->stringList().at(idx.row()), mpStream));

    updateSelectProtocolsAdvancedWidget();
    lvSelectedProtocols->setCurrentIndex(idx2);
}

void StreamConfigDialog::on_tbDelete_clicked()
{
    int n;
    QModelIndex idx;
    AbstractProtocol *p = NULL;

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

    Q_CHECK_PTR(p);
    _iter->remove();
    delete p;

    updateSelectProtocolsAdvancedWidget();
    lvSelectedProtocols->setCurrentIndex(idx);
}

void StreamConfigDialog::on_tbUp_clicked()
{
    int m, n;
    QModelIndex idx;
    AbstractProtocol *p = NULL;

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

    Q_CHECK_PTR(p);
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
    AbstractProtocol *p = NULL;

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

    Q_CHECK_PTR(p);
    _iter->remove();
    _iter->next();
    _iter->insert(p);

    updateSelectProtocolsAdvancedWidget();
    lvSelectedProtocols->setCurrentIndex(idx.sibling(m,0));
}

void StreamConfigDialog::updateSelectProtocolsAdvancedWidget()
{
    QStringList    selProtoList;

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
            // Hide the ToolBox before modifying it - else we have a crash !!!
            tbProtocolData->hide();

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

            if (lastProtocolDataIndex < tbProtocolData->count())
                tbProtocolData->setCurrentIndex(lastProtocolDataIndex);

            tbProtocolData->show();
            break;
        }

        // Stream Control
        case 2:
        {
            StoreCurrentStream();
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

    lastProtocolDataIndex = tbProtocolData->currentIndex();
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
    ulong    num = 0;
    bool    isOk;
    QString    str;

    num = lePattern->text().remove(QChar(' ')).toULong(&isOk, 16);
    qDebug("editfinished (%s | %x)\n", lePattern->text().toAscii().data(), num);
    lePattern->setText(uintToHexStr(num, str, 4));
    qDebug("editfinished (%s | %x)\n", lePattern->text().toAscii().data(), num);
}
#endif

/*! 
Skip protocols upto and including the layer specified.
*/
bool StreamConfigDialog::skipProtocols(int layer)
{
    _iter->toFront();

    for (int i = ProtoMin; i <= layer; i++)
    {
        if(_iter->hasNext())
        {
            int id;
            QAbstractButton *btn;

            id = _iter->peekNext()->protocolNumber();
            btn = bgProto[i]->button(id);
            if (btn)
                _iter->next();
        }
    }

    return true;
}

/*!
Protocol choices (except "None" and "Other") for a protocol button group are disabled if checked is true, else they are enabled
*/
void StreamConfigDialog::disableProtocols(QButtonGroup *protocolGroup, bool checked)
{
    qDebug("%s: btnGrp = %p, chk? = %d", __FUNCTION__, protocolGroup, checked);
    foreach(QAbstractButton *btn, protocolGroup->buttons())
    {
        int id = protocolGroup->id(btn);

        if ((id != ButtonIdNone) && (id != ButtonIdOther))
            btn->setDisabled(checked);
    }
}

void StreamConfigDialog::forceProtocolNone(bool checked)
{
    QObject *btn;

    btn = sender();
    Q_ASSERT(btn != NULL);

    qDebug("%s: chk? = %d, btn = %p, L1 = %p, L2 = %p, L3 = %p", __FUNCTION__,
            checked, btn, rbL1None, rbFtNone, rbL3None);

    if (btn == rbL1None)
    {
        if (checked)
        {
            bgProto[ProtoVlan]->button(ButtonIdNone)->click();
            bgProto[ProtoL2]->button(ButtonIdNone)->click();
            bgProto[ProtoPayload]->button(ButtonIdNone)->click();
        }

        disableProtocols(bgProto[ProtoVlan], checked);
        disableProtocols(bgProto[ProtoL2], checked);
        disableProtocols(bgProto[ProtoPayload], checked);
    } 
    else if (btn == rbFtNone)
    {
        if (checked)
            bgProto[ProtoL3]->button(ButtonIdNone)->click();
        disableProtocols(bgProto[ProtoL3], checked);
    }
    else if (btn == rbL3None)
    {
        if (checked)
            bgProto[ProtoL4]->button(ButtonIdNone)->click();
        disableProtocols(bgProto[ProtoL4], checked);
    }
    else if (btn == rbL4None)
    {
        if (checked)
            bgProto[ProtoL5]->button(ButtonIdNone)->click();
        disableProtocols(bgProto[ProtoL5], checked);
    }
    else
    {
        Q_ASSERT(1 == 0); // Unreachable code!
    }
}

void StreamConfigDialog::updateProtocol(int newId)
{
    int level;
    QButtonGroup    *btnGrp;

    btnGrp = static_cast<QButtonGroup*>(sender());
    Q_ASSERT(btnGrp != NULL);

    level = btnGrp->property("ProtocolLevel").toInt();
    Q_ASSERT(btnGrp == bgProto[level]);

    __updateProtocol(level, newId);
}

void StreamConfigDialog::__updateProtocol(int level, int newId)
{
    int oldId;
    QButtonGroup    *btnGrp;

    Q_ASSERT((level >= ProtoMin) && (level <= ProtoMax));
    btnGrp = bgProto[level];
    oldId = btnGrp->property("ProtocolId").toInt();

    qDebug("%s: level = %d old id = %d new id = %d upd? = %d", __FUNCTION__, 
        level, oldId, newId, isUpdateInProgress);

    if (newId == oldId)
        return;

    if (!isUpdateInProgress)
    {
        int ret;
        AbstractProtocol *p;

        ret = skipProtocols(level-1);
        Q_ASSERT(ret == true);

        Q_ASSERT(oldId != newId);
        Q_ASSERT(newId != ButtonIdOther);
        
        switch (oldId)
        {
            case ButtonIdNone:
                _iter->insert(OstProtocolManager->createProtocol(
                        newId, mpStream));
                break;

            case ButtonIdOther:
            default:
                Q_ASSERT(_iter->hasNext());
                p =_iter->next();

                if (newId)
                    _iter->setValue(OstProtocolManager->createProtocol(
                            newId, mpStream));
                else
                    _iter->remove();
                delete p;
                if (level == ProtoPayload)
                {
                    while (_iter->hasNext())
                    {
                        p = _iter->next();
                        _iter->remove();
                        delete p;
                    }
                }
                break;
        }
    }

    btnGrp->setProperty("ProtocolId", newId);
    return;
}

void StreamConfigDialog::updateSelectProtocolsSimpleWidget()
{
    int i;
    quint32    id;
    QAbstractButton *btn;

    qDebug("%s", __FUNCTION__);

    isUpdateInProgress = true;

    // Reset to default state ...
    for (i = ProtoMin; i < ProtoMax; i++)
        bgProto[i]->button(ButtonIdNone)->click();

    // ... now iterate and update
    _iter->toFront();

    for (i = ProtoMin; i < ProtoMax; i++)
    {
        if (!_iter->hasNext())
            goto _done;

        id = _iter->next()->protocolNumber();
        btn = bgProto[i]->button(id);

        if (btn)
        {
            if (btn->isEnabled())
                btn->click();
            else
            {
                btn->setChecked(true);
                __updateProtocol(i, id);
            }
        }
        else
        {
            switch (i)
            {
                case ProtoVlan:
                    _iter->previous();
                    break;

                case ProtoPayload:
                    goto _other;

                default:
                    btn = bgProto[ProtoPayload]->button(id);
                    if (btn && btn->isEnabled())
                    {
                        btn->click();
                        break;
                    }
                    else
                        goto _other;
            }
        }
    }

    // If more protocol(s) beyond payload ...
    if (_iter->hasNext())
    {
        i = ProtoPayload;
        goto _other;
    }

    goto _done;

_other:
    for (int j = i; j < ProtoMax; j++)
    {
        // VLAN doesn't have a "Other" button
        if (j == ProtoVlan)
            continue;

        bgProto[j]->button(ButtonIdOther)->setChecked(true);
        __updateProtocol(j, ButtonIdOther);
    }

_done:
    isUpdateInProgress = false;
}

void StreamConfigDialog::LoadCurrentStream()
{
    QString    str;

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
        lePacketsPerSec->setText(
                QString("%L1").arg(mpStream->packetRate(), 0, 'f', 4));
        leBurstsPerSec->setText(
                QString("%L1").arg(mpStream->burstRate(), 0, 'f', 4));
        // TODO(MED): Change this when we support goto to specific stream
        leStreamId->setText(QString("0"));

        leGapIsg->setText("0.0");
    }
    qDebug("loading stream done");
}

void StreamConfigDialog::StoreCurrentStream()
{
    QString    str;
    bool    isOk;
    Stream    *pStream = mpStream;

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
        pStream->setPacketRate(
                QLocale().toDouble(lePacketsPerSec->text(), &isOk));
        pStream->setBurstRate(
                QLocale().toDouble(leBurstsPerSec->text(), &isOk));
    }
}

void StreamConfigDialog::on_tbProtocolData_currentChanged(int /*index*/)
{
    // Refresh protocol widgets in case there is any dependent data between 
    // protocols e.g. TCP/UDP port numbers are dependent on Port/Protocol 
    // selection in TextProtocol
#if 0 // FIXME: temp mask to avoid crash till we fix it
    mpStream->storeProtocolWidgets();
    mpStream->loadProtocolWidgets();
#endif
}

void StreamConfigDialog::on_rbPacketsPerSec_toggled(bool checked)
{
    if (checked)
        on_lePacketsPerSec_textChanged(lePacketsPerSec->text());
}

void StreamConfigDialog::on_rbBurstsPerSec_toggled(bool checked)
{
    if (checked)
        on_leBurstsPerSec_textChanged(leBurstsPerSec->text());
}

void StreamConfigDialog::on_lePacketsPerBurst_textChanged(const QString &text)
{
    if (rbSendBursts->isChecked())
        on_leBurstsPerSec_textChanged(leBurstsPerSec->text());
}

void StreamConfigDialog::on_lePacketsPerSec_textChanged(const QString &text)
{
    bool isOk;
    Stream *pStream = mpStream;
    uint frameLen;

    if (pStream->lenMode() == Stream::e_fl_fixed)
        frameLen = pStream->frameLen();
    else
        frameLen = (pStream->frameLenMin() + pStream->frameLenMax())/2;

    if (rbSendPackets->isChecked())
    {
        double pktsPerSec = QLocale().toDouble(text, &isOk);
        double bitsPerSec = pktsPerSec * double((frameLen+kEthFrameOverHead)*8);

        if (rbPacketsPerSec->isChecked())
            leBitsPerSec->setText(QString("%L1").arg(bitsPerSec, 0, 'f', 0));
        leGapIbg->setText(QString("0.0"));
        leGapIpg->setText(QString("%L1").arg(1/double(pktsPerSec), 0, 'f', 9));
    }
}

void StreamConfigDialog::on_leBurstsPerSec_textChanged(const QString &text)
{
    bool isOk;
    Stream *pStream = mpStream;
    uint burstSize = lePacketsPerBurst->text().toULong(&isOk);
    uint frameLen;

    qDebug("start of %s(%s)", __FUNCTION__, text.toAscii().constData());
    if (pStream->lenMode() == Stream::e_fl_fixed)
        frameLen = pStream->frameLen();
    else
        frameLen = (pStream->frameLenMin() + pStream->frameLenMax())/2;

    if (rbSendBursts->isChecked())
    {
        double burstsPerSec = QLocale().toDouble(text, &isOk);
        double bitsPerSec = burstsPerSec *
                double(burstSize * (frameLen + kEthFrameOverHead) * 8);
        if (rbBurstsPerSec->isChecked())
            leBitsPerSec->setText(QString("%L1").arg(bitsPerSec, 0, 'f', 0));
        leGapIbg->setText(QString("%L1").arg(1/double(burstsPerSec), 0, 'f',9));
        leGapIpg->setText(QString("0.0"));
    }
    qDebug("end of %s", __FUNCTION__);
}

void StreamConfigDialog::on_leBitsPerSec_textEdited(const QString &text)
{
    bool isOk;
    Stream *pStream = mpStream;
    uint burstSize = lePacketsPerBurst->text().toULong(&isOk);
    uint frameLen;

    if (pStream->lenMode() == Stream::e_fl_fixed)
        frameLen = pStream->frameLen();
    else
        frameLen = (pStream->frameLenMin() + pStream->frameLenMax())/2;

    if (rbSendPackets->isChecked())
    {
        double pktsPerSec = QLocale().toDouble(text, &isOk)/
                double((frameLen+kEthFrameOverHead)*8);
        lePacketsPerSec->setText(QString("%L1").arg(pktsPerSec, 0, 'f', 4));
    }
    else if (rbSendBursts->isChecked())
    {
        double burstsPerSec = QLocale().toDouble(text, &isOk)/
                double(burstSize * (frameLen + kEthFrameOverHead) * 8);
        leBurstsPerSec->setText(QString("%L1").arg(burstsPerSec, 0, 'f', 4));
    }
}

void StreamConfigDialog::on_pbOk_clicked()
{
    QString log;
    OstProto::Stream s;

    // Store dialog contents into stream
    StoreCurrentStream();

    if ((mPort.transmitMode() == OstProto::kInterleavedTransmit)
            && (mpStream->isFrameVariable()))
    {
        log += "In 'Interleaved Streams' transmit mode, the count for "
            "varying fields at transmit time may not be same as configured\n";
    }

    mpStream->preflightCheck(log);

    if (log.length())
    {
        if (QMessageBox::warning(this, "Preflight Check", log + "\nContinue?",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No) 
                == QMessageBox::No)
            return;
    }

    // Copy the data from the "local working copy of stream" to "actual stream"
    mpStream->protoDataCopyInto(s);
    mPort.streamByIndex(mCurrentStreamIndex)->protoDataCopyFrom(s);

    qDebug("stream stored");

    lastGeometry = geometry();
    lastTopLevelTabIndex = twTopLevel->currentIndex();
    lastProtocolDataIndex = tbProtocolData->currentIndex();

    accept();
}

