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

PcapFileFormat::PcapFileFormat()
{
}

PcapFileFormat::~PcapFileFormat()
{
}

bool PcapFileFormat::openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error)
{
    bool viaPdml = true; // TODO: shd be a param to function

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
    QByteArray pktBuf;

    if (!file.open(QIODevice::ReadOnly))
        goto _err_open;

    len = file.peek((char*)gzipMagic, sizeof(gzipMagic));
    if (len < int(sizeof(gzipMagic)))
        goto _err_reading_magic;

    if ((gzipMagic[0] == 0x1f) && (gzipMagic[1] == 0x8b))
    {
        QProcess gzip;

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

    if (viaPdml)
    {
        QTemporaryFile pdmlFile;
        PdmlReader reader(&streams);
        QProcess tshark;

        if (!pdmlFile.open())
        {
            error.append("Unable to open temporary file to create PDML\n");
            goto _non_pdml;
        }

        qDebug("generating PDML %s", pdmlFile.fileName().toAscii().constData());

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

        isOk = reader.read(&pdmlFile, this); // TODO: pass error string?
        goto _exit;
       
    }

_non_pdml:
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
    }

    isOk = true;
    goto _exit;

#if 1
_err_unsupported_encap:
    error = QString(tr("%1 has non-ethernet encapsulation (%2) which is "
                "not supported - Sorry!"))
            .arg(QFileInfo(fileName).fileName()).arg(fileHdr.network);
    goto _exit;
#endif

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
