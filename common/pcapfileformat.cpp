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

#include "pcapfileformat.h"

#include "pdml_p.h"
#include "streambase.h"
#include "hexdump.pb.h"

#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QtGlobal>

static inline quint32 swap32(quint32 val)
{
    return (((val >> 24) && 0x000000FF) |
            ((val >> 16) && 0x0000FF00) |
            ((val << 16) && 0x00FF0000) |
            ((val << 24) && 0xFF000000));
}

static inline quint16 swap16(quint16 val)
{
    return (((val >> 8) && 0x00FF) |
            ((val << 8) && 0xFF00));
}

const quint32 kPcapFileMagic = 0xa1b2c3d4;
const quint32 kPcapFileMagicSwapped = 0xd4c3b2a1;
const quint16 kPcapFileVersionMajor = 2;
const quint16 kPcapFileVersionMinor = 4;
const quint32 kMaxSnapLen = 65535;
const quint32 kDltEthernet = 1;

PcapFileFormat pcapFileFormat;

PcapImportOptionsDialog::PcapImportOptionsDialog(QVariantMap *options)
    : QDialog(NULL)
{
    setupUi(this);
    options_ = options;

    viaPdml->setChecked(options_->value("ViaPdml").toBool());
    doDiff->setChecked(options_->value("DoDiff").toBool());

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

PcapImportOptionsDialog::~PcapImportOptionsDialog()
{
}

void PcapImportOptionsDialog::accept()
{
    options_->insert("ViaPdml", viaPdml->isChecked());
    options_->insert("DoDiff", doDiff->isChecked());

    QDialog::accept();
}

PcapFileFormat::PcapFileFormat()
{
    importOptions_.insert("ViaPdml", true);
    importOptions_.insert("DoDiff", true);

    importDialog_ = NULL;
}

PcapFileFormat::~PcapFileFormat()
{
    delete importDialog_;
}

bool PcapFileFormat::openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error)
{
    bool isOk = false;
    QFile file(fileName);
    QTemporaryFile file2;
    quint32 magic;
    uchar gzipMagic[2];
    int len;
    PcapFileHeader fileHdr;
    PcapPacketHeader pktHdr;
    OstProto::Stream *prevStream = NULL;
    uint lastUsec = 0;
    int pktCount;
    qint64 byteCount = 0;
    qint64 byteTotal;
    QByteArray pktBuf;

    if (!file.open(QIODevice::ReadOnly))
        goto _err_open;

    len = file.peek((char*)gzipMagic, sizeof(gzipMagic));
    if (len < int(sizeof(gzipMagic)))
        goto _err_reading_magic;

    if ((gzipMagic[0] == 0x1f) && (gzipMagic[1] == 0x8b))
    {
        QProcess gzip;

        emit status("Decompressing...");
        emit target(0);

        if (!file2.open())
        {
            error.append("Unable to open temporary file to uncompress .gz\n");
            goto _err_unzip_fail;
        }

        qDebug("decompressing to %s", file2.fileName().toAscii().constData());

        gzip.setStandardOutputFile(file2.fileName());
        // FIXME: hardcoded prog name
        gzip.start("C:/Program Files/CmdLineTools/gzip.exe", 
                QStringList() 
                << "-d" 
                << "-c" 
                << fileName);
        if (!gzip.waitForStarted(-1))
        {
            error.append(QString("Unable to start gzip\n"));
            goto _err_unzip_fail;
        }

        if (!gzip.waitForFinished(-1))
        {
            error.append(QString("Error running gzip\n"));
            goto _err_unzip_fail;
        }

        file2.seek(0);

        fd_.setDevice(&file2);
    }
    else
    {
        fd_.setDevice(&file);
    }

    byteTotal = fd_.device()->size() - sizeof(fileHdr);

    emit status("Reading File Header...");
    emit target(0);

    fd_ >> magic;

    qDebug("magic = %08x", magic);

    if (magic == kPcapFileMagicSwapped)
    {
        // Toggle Byte order
        if (fd_.byteOrder() == QDataStream::BigEndian)
            fd_.setByteOrder(QDataStream::LittleEndian);
        else
            fd_.setByteOrder(QDataStream::BigEndian);
    }
    else if (magic != kPcapFileMagic)
        goto _err_bad_magic;

    fd_ >> fileHdr.versionMajor;
    fd_ >> fileHdr.versionMinor;
    fd_ >> fileHdr.thisZone;
    fd_ >> fileHdr.sigfigs;
    fd_ >> fileHdr.snapLen;
    fd_ >> fileHdr.network;
   
    if ((fileHdr.versionMajor != kPcapFileVersionMajor) ||
            (fileHdr.versionMinor != kPcapFileVersionMinor))
        goto _err_unsupported_version;

#if 1
    // XXX: we support only Ethernet, for now
    if (fileHdr.network != kDltEthernet)
        goto _err_unsupported_encap;
#endif

    pktBuf.resize(fileHdr.snapLen);

    if (importOptions_.value("ViaPdml").toBool())
    {
        QProcess tshark;
        QTemporaryFile pdmlFile;
        PdmlReader reader(&streams);

        if (!pdmlFile.open())
        {
            error.append("Unable to open temporary file to create PDML\n");
            goto _non_pdml;
        }

        qDebug("generating PDML %s", pdmlFile.fileName().toAscii().constData());
        emit status("Generating PDML...");
        emit target(0);

        tshark.setStandardOutputFile(pdmlFile.fileName());
        // FIXME: hardcoded prog name
        tshark.start("C:/Program Files/Wireshark/Tshark.exe", 
                QStringList() 
                << QString("-r%1").arg(fileName)
                << "-otcp.desegment_tcp_streams:FALSE"
                << "-Tpdml");
        if (!tshark.waitForStarted(-1))
        {
            error.append(QString("Unable to start tshark\n"));
            goto _non_pdml;
        }

        if (!tshark.waitForFinished(-1))
        {
            error.append(QString("Error running tshark\n"));
            goto _non_pdml;
        }

        connect(&reader, SIGNAL(progress(int)), this, SIGNAL(progress(int)));

        emit status("Reading PDML packets...");
        emit target(100); // in percentage
        isOk = reader.read(&pdmlFile, this, &stop_);
        
        if (stop_)
            goto _user_cancel;

        if (!isOk)
        {
            error.append(QString("Error processing PDML (%1, %2): %3\n")
                    .arg(reader.lineNumber())
                    .arg(reader.columnNumber())
                    .arg(reader.errorString()));
            goto _exit;
        }

        if (!importOptions_.value("DoDiff").toBool())
            goto _exit;


        // !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!
        //             Let's do the diff ...
        // !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!

        QProcess awk;
        QProcess diff;
        QTemporaryFile originalTextFile;
        QTemporaryFile importedPcapFile;
        QTemporaryFile importedTextFile;
        QTemporaryFile diffFile;
        const QString kAwkFilter =
            "/^[^0]/ { "
                        "printf \" %s \", $1;"
                        "for (i=4; i<NF; i++) printf \"%s \", $i;"
                        "next;"
                    "}"
            "// {print}";

        // Convert original file to text ...
        if (!originalTextFile.open())
        {
            error.append("Unable to open temporary file to create text file "
                    "(original) for diff\n");
            goto _diff_fail;
        }
        qDebug("generating text file (original) %s", 
                originalTextFile.fileName().toAscii().constData());

        emit status("Preparing original PCAP for diff...");
        emit target(0);

        tshark.setStandardOutputProcess(&awk);
        awk.setStandardOutputFile(originalTextFile.fileName());
        // FIXME: hardcoded prog name
        tshark.start("C:/Program Files/Wireshark/Tshark.exe", 
                QStringList() 
                << QString("-r%1").arg(fileName)
                << "-otcp.desegment_tcp_streams:FALSE"
                << "-x");
        if (!tshark.waitForStarted(-1))
        {
            error.append(QString("Unable to start tshark - %1\n")
                    .arg(tshark.exitCode()));
            goto _diff_fail;
        }

        // FIXME: hardcoded prog name
        awk.start("D:/srivatsp/projects/ostinato/pdml/bin/gawk.exe", 
                QStringList() << kAwkFilter);
        if (!awk.waitForStarted(-1))
        {
            tshark.kill();
            error.append(QString("Unable to start awk - %1\n")
                    .arg(awk.exitCode()));
            goto _diff_fail;
        }

        if (!tshark.waitForFinished(-1))
        {
            error.append(QString("Error running tshark\n"));
            goto _diff_fail;
        }
        if (!awk.waitForFinished(-1))
        {
            error.append(QString("Error running awk\n"));
            goto _diff_fail;
        }

        // Save imported file as PCAP
        if (!importedPcapFile.open())
        {
            error.append("Unable to open temporary file to create pcap file "
                    "from imported streams for diff\n");
            goto _diff_fail;
        }

        if (!saveStreams(streams, importedPcapFile.fileName(), error))
        {
            error.append("Error saving imported streams as PCAP for diff");
            goto _diff_fail;
        }

        // Convert imported file to text ...
        if (!importedTextFile.open())
        {
            error.append("Unable to open temporary file to create text file "
                    "(imported) for diff\n");
            goto _diff_fail;
        }
        qDebug("generating text file (imported) %s", 
                importedTextFile.fileName().toAscii().constData());

        emit status("Preparing imported PCAP for diff...");
        emit target(0);

        tshark.setStandardOutputProcess(&awk);
        awk.setStandardOutputFile(importedTextFile.fileName());
        // FIXME: hardcoded prog name
        tshark.start("C:/Program Files/Wireshark/Tshark.exe", 
                QStringList() 
                << QString("-r%1").arg(importedPcapFile.fileName())
                << "-otcp.desegment_tcp_streams:FALSE"
                << "-x");
        if (!tshark.waitForStarted(-1))
        {
            error.append(QString("Unable to start tshark - %1\n")
                    .arg(tshark.exitCode()));
            goto _diff_fail;
        }

        // FIXME: hardcoded prog name
        awk.start("D:/srivatsp/projects/ostinato/pdml/bin/gawk.exe", 
                QStringList() << kAwkFilter);
        if (!awk.waitForStarted(-1))
        {
            tshark.kill();
            error.append(QString("Unable to start awk - %1\n")
                    .arg(awk.exitCode()));
            goto _diff_fail;
        }

        if (!tshark.waitForFinished(-1))
        {
            error.append(QString("Error running tshark\n"));
            goto _diff_fail;
        }
        if (!awk.waitForFinished(-1))
        {
            error.append(QString("Error running awk\n"));
            goto _diff_fail;
        }

        // Now do the diff of the two text files ...
        if (!diffFile.open())
        {
            error.append("Unable to open temporary file to store diff\n");
            goto _diff_fail;
        }
        qDebug("diffing %s and %s > %s", 
                originalTextFile.fileName().toAscii().constData(),
                importedTextFile.fileName().toAscii().constData(),
                diffFile.fileName().toAscii().constData());

        emit status("Taking diff...");
        emit target(0);

        diff.setStandardOutputFile(diffFile.fileName());
        // FIXME: hardcoded prog name
        diff.start("D:/srivatsp/projects/ostinato/pdml/bin/diff.exe", 
                QStringList()
                << "-u"
                << "-F^ [1-9]"
                << QString("--label=%1 (actual)")
                    .arg(QFileInfo(fileName).fileName())
                << QString("--label=%1 (imported)")
                    .arg(QFileInfo(fileName).fileName())
                << originalTextFile.fileName()
                << importedTextFile.fileName());
        if (!diff.waitForStarted(-1))
        {
            error.append(QString("Unable to start diff\n")
                    .arg(diff.exitCode()));
            goto _diff_fail;
        }

        if (!diff.waitForFinished(-1))
        {
            error.append(QString("Error running diff\n"));
            goto _diff_fail;
        }

        diffFile.close();
        if (diffFile.size())
        {
            error.append("There is a diff between the original and imported streams. See details for diff.\n\n\n\n");
            diffFile.open();
            diffFile.seek(0);
            error.append(QString(diffFile.readAll()));
        }

        goto _exit;
    }

_non_pdml:
    emit status("Reading Packets...");
    emit target(100);  // in percentage
    pktCount = 1;
    while (!fd_.atEnd())
    {
        OstProto::Stream *stream = streams.add_stream();
        OstProto::Protocol *proto = stream->add_protocol();
        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        readPacket(pktHdr, pktBuf);

        // validations on inclLen <= origLen && inclLen <= snapLen
        Q_ASSERT(pktHdr.inclLen <= fileHdr.snapLen); // TODO: convert to if

        hexDump->set_content(pktBuf.data(), pktHdr.inclLen);
        hexDump->set_pad_until_end(false);

        stream->mutable_stream_id()->set_id(pktCount);
        stream->mutable_core()->set_is_enabled(true);
        stream->mutable_core()->set_frame_len(pktHdr.inclLen+4); // FCS

        // setup packet rate to the timing in pcap (as close as possible)
        const uint kUsecsInSec = uint(1e6);
        uint usec = (pktHdr.tsSec*kUsecsInSec + pktHdr.tsUsec);
        uint delta = usec - lastUsec;
        
        if ((pktCount != 1) && delta)
            stream->mutable_control()->set_packets_per_sec(kUsecsInSec/delta);

        if (prevStream)
            prevStream->mutable_control()->CopyFrom(stream->control());

        lastUsec = usec;
        prevStream = stream;
        pktCount++;
        qDebug("pktCount = %d", pktCount);
        byteCount += pktHdr.inclLen + sizeof(pktHdr);
        emit progress(int(byteCount*100/byteTotal)); // in percentage
        if (stop_)
            goto _user_cancel;
    }

    isOk = true;
    goto _exit;

_user_cancel:
    isOk = true;
    goto _exit;

_diff_fail:
    goto _exit;

_err_unsupported_encap:
    error = QString(tr("%1 has non-ethernet encapsulation (%2) which is "
                "not supported - Sorry!"))
            .arg(QFileInfo(fileName).fileName()).arg(fileHdr.network);
    goto _exit;

_err_unsupported_version:
    error = QString(tr("%1 is in PCAP version %2.%3 format which is "
                "not supported - Sorry!"))
            .arg(fileName).arg(fileHdr.versionMajor).arg(fileHdr.versionMinor);
    goto _exit;

_err_bad_magic:
    error = QString(tr("%1 is not a valid PCAP file")).arg(fileName);
    goto _exit;

#if 0
_err_truncated:
    error = QString(tr("%1 is too short")).arg(fileName);
    goto _exit;
#endif 

_err_unzip_fail:
    goto _exit;

_err_reading_magic:
    error = QString(tr("Unable to read magic from %1")).arg(fileName);
    goto _exit;

_err_open:
    error = QString(tr("Unable to open file: %1")).arg(fileName);
    goto _exit;

_exit:
    file.close();
    return isOk;
}

/*!
  Reads packet meta data into pktHdr and packet content into buf.

  Returns true if packet is read successfully, false otherwise.
*/
bool PcapFileFormat::readPacket(PcapPacketHeader &pktHdr, QByteArray &pktBuf)
{
    quint32 len;

    // TODO: chk fd_.status()

    // read PcapPacketHeader
    fd_ >> pktHdr.tsSec;
    fd_ >> pktHdr.tsUsec;
    fd_ >> pktHdr.inclLen;
    fd_ >> pktHdr.origLen;

    // TODO: chk fd_.status()

    // XXX: should never be required, but we play safe
    if (quint32(pktBuf.size()) < pktHdr.inclLen)
        pktBuf.resize(pktHdr.inclLen);

    // read Pkt contents
    len = fd_.readRawData(pktBuf.data(), pktHdr.inclLen); // TODO: use while?

    Q_ASSERT(len == pktHdr.inclLen); // TODO: remove assert
    pktBuf.resize(len);

    return true;
}

bool PcapFileFormat::saveStreams(const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    bool isOk = false;
    QFile file(fileName);
    PcapFileHeader fileHdr;
    PcapPacketHeader pktHdr;
    QByteArray pktBuf;

    if (!file.open(QIODevice::WriteOnly))
        goto _err_open;

    fd_.setDevice(&file);

    fileHdr.magicNumber = kPcapFileMagic;
    fileHdr.versionMajor = kPcapFileVersionMajor;
    fileHdr.versionMinor = kPcapFileVersionMinor;
    fileHdr.thisZone = 0;
    fileHdr.sigfigs = 0;
    fileHdr.snapLen = kMaxSnapLen;
    fileHdr.network = kDltEthernet; 

    fd_ << fileHdr.magicNumber;
    fd_ << fileHdr.versionMajor;
    fd_ << fileHdr.versionMinor;
    fd_ << fileHdr.thisZone;
    fd_ << fileHdr.sigfigs;
    fd_ << fileHdr.snapLen;
    fd_ << fileHdr.network;

    pktBuf.resize(kMaxSnapLen);

    emit status("Writing Packets...");
    emit target(streams.stream_size());

    pktHdr.tsSec = 0;
    pktHdr.tsUsec = 0;
    for (int i = 0; i < streams.stream_size(); i++)
    {
        StreamBase s;
        
        s.setId(i);
        s.protoDataCopyFrom(streams.stream(i));
        // TODO: expand frameIndex for each stream
        s.frameValue((uchar*)pktBuf.data(), pktBuf.size(), 0); 

        pktHdr.inclLen = s.frameProtocolLength(0); // FIXME: stream index = 0
        pktHdr.origLen = s.frameLen() - 4; // FCS; FIXME: Hardcoding

        qDebug("savepcap i=%d, incl/orig len = %d/%d", i, 
                pktHdr.inclLen, pktHdr.origLen);

        if (pktHdr.inclLen > fileHdr.snapLen)
            pktHdr.inclLen = fileHdr.snapLen;

        fd_ << pktHdr.tsSec;
        fd_ << pktHdr.tsUsec;
        fd_ << pktHdr.inclLen;
        fd_ << pktHdr.origLen;
        fd_.writeRawData(pktBuf.data(), pktHdr.inclLen);

        if (s.packetRate())
            pktHdr.tsUsec += 1000000/s.packetRate();
        if (pktHdr.tsUsec >= 1000000)
        {
            pktHdr.tsSec++;
            pktHdr.tsUsec -= 1000000;
        }

        emit progress(i);
    }

    file.close();

    isOk = true;
    goto _exit;

_err_open:
    error = QString(tr("Unable to open file: %1")).arg(fileName);
    goto _exit;

_exit:
    return isOk;
}

QDialog* PcapFileFormat::openOptionsDialog()
{
    if (!importDialog_)
        importDialog_ = new PcapImportOptionsDialog(&importOptions_);

    return importDialog_;
}

bool PcapFileFormat::isMyFileFormat(const QString fileName)
{
    // TODO
    return true;
}

bool PcapFileFormat::isMyFileType(const QString fileType)
{
    if (fileType.startsWith("PCAP"))
        return true;
    else
        return false;
}
