/*
Copyright (C) 2011 Srivats P.

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
#ifndef _PCAP_FILE_FORMAT_H
#define _PCAP_FILE_FORMAT_H

#include "abstractfileformat.h"
#include "ui_pcapfileimport.h"

#include <QDataStream>
#include <QVariantMap>

class PcapImportOptionsDialog: public QDialog, public Ui::PcapFileImport
{
public: 
    PcapImportOptionsDialog(QVariantMap *options);
    ~PcapImportOptionsDialog();

private slots:
    void accept();

private:
    QVariantMap *options_;
};

class PdmlReader;
class PcapFileFormat : public AbstractFileFormat
{
    friend class PdmlReader;

public:
    PcapFileFormat();
    ~PcapFileFormat();

    bool openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error);
    bool saveStreams(const OstProto::StreamConfigList streams, 
            const QString fileName, QString &error);

    virtual QDialog* openOptionsDialog();

    bool isMyFileFormat(const QString fileName);
    bool isMyFileType(const QString fileType);

private:
    typedef struct {
        quint32 magicNumber;   /* magic number */
        quint16 versionMajor;  /* major version number */
        quint16 versionMinor;  /* minor version number */
        qint32  thisZone;      /* GMT to local correction */
        quint32 sigfigs;       /* accuracy of timestamps */
        quint32 snapLen;       /* max length of captured packets, in octets */
        quint32 network;       /* data link type */
    } PcapFileHeader;

    typedef struct {
        quint32 tsSec;         /* timestamp seconds */
        quint32 tsUsec;        /* timestamp microseconds */
        quint32 inclLen;       /* number of octets of packet saved in file */
        quint32 origLen;       /* actual length of packet */
    } PcapPacketHeader;

    bool readPacket(PcapPacketHeader &pktHdr, QByteArray &pktBuf);

    QDataStream fd_;
    QVariantMap importOptions_;
    PcapImportOptionsDialog *importDialog_;
};

extern PcapFileFormat pcapFileFormat;

#endif
