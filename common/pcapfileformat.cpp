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

#include "pcapfileformat.h"
#include "streambase.h"
#include "hexdump.pb.h"

#include <QDataStream>
#include <QFile>
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

typedef struct {
    quint32 magicNumber;   /* magic number */
    quint16 versionMajor;  /* major version number */
    quint16 versionMinor;  /* minor version number */
    qint32  thisZone;      /* GMT to local correction */
    quint32 sigfigs;       /* accuracy of timestamps */
    quint32 snapLen;       /* max length of captured packets, in octets */
    quint32 network;       /* data link type */
} PcapFileHeader;

const quint32 kPcapFileMagic = 0xa1b2c3d4;
const quint32 kPcapFileMagicSwapped = 0xd4c3b2a1;
const quint16 kPcapFileVersionMajor = 2;
const quint16 kPcapFileVersionMinor = 4;
const quint32 kMaxSnapLen = 65535;

typedef struct {
    quint32 tsSec;         /* timestamp seconds */
    quint32 tsUsec;        /* timestamp microseconds */
    quint32 inclLen;       /* number of octets of packet saved in file */
    quint32 origLen;       /* actual length of packet */
} PcapPacketHeader;

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
    bool isOk = false;
    QFile file(fileName);
    QDataStream fd;
    quint32 magic;
    PcapFileHeader fileHdr;
    PcapPacketHeader pktHdr;
    quint32 len;
    int pktCount = 1;
    QByteArray pktBuf;

    if (!file.open(QIODevice::ReadOnly))
        goto _err_open;

    if (file.size() < sizeof(fileHdr))
        goto _err_truncated;

    fd.setDevice(&file);

    fd >> magic;


    if (magic == kPcapFileMagicSwapped)
    {
        // Toggle Byte order
        if (fd.byteOrder() == QDataStream::BigEndian)
            fd.setByteOrder(QDataStream::LittleEndian);
        else
            fd.setByteOrder(QDataStream::BigEndian);
    }
    else if (magic != kPcapFileMagic)
        goto _err_bad_magic;

    fd >> fileHdr.versionMajor;
    fd >> fileHdr.versionMinor;
    fd >> fileHdr.thisZone;
    fd >> fileHdr.sigfigs;
    fd >> fileHdr.snapLen;
    fd >> fileHdr.network;
   
    if ((fileHdr.versionMajor != kPcapFileVersionMajor) ||
            (fileHdr.versionMinor != kPcapFileVersionMinor))
        goto _err_unsupported_version;

    // TODO: what do we do about non-ethernet networks?

    pktBuf.resize(fileHdr.snapLen);

    while (!fd.atEnd())
    {
        OstProto::Stream *stream = streams.add_stream();
        OstProto::Protocol *proto = stream->add_protocol();
        OstProto::HexDump *hexDump = proto->MutableExtension(OstProto::hexDump);

        stream->mutable_stream_id()->set_id(pktCount);
        stream->mutable_core()->set_is_enabled(true);
        proto->mutable_protocol_id()->set_id(
                OstProto::Protocol::kHexDumpFieldNumber);

        // read PcapPacketHeader
        fd >> pktHdr.tsSec;
        fd >> pktHdr.tsUsec;
        fd >> pktHdr.inclLen;
        fd >> pktHdr.origLen;

        // TODO: chk fd.status()

        // validations on inclLen <= origLen && inclLen <= snapLen
        Q_ASSERT(pktHdr.inclLen <= fileHdr.snapLen); // TODO: convert to if

        // read Pkt contents
        len = fd.readRawData(pktBuf.data(), pktHdr.inclLen); // TODO: use while?
        Q_ASSERT(len == pktHdr.inclLen); // TODO: remove assert

        hexDump->set_content(pktBuf.data(), pktHdr.inclLen);
        hexDump->set_pad_until_end(false);

        stream->mutable_core()->set_frame_len(pktHdr.inclLen+4); // FCS

        pktCount++;
    }

    isOk = true;
    goto _exit;

_err_unsupported_version:
    error = QString(tr("%1 is in PCAP version %2.%3 format which is "
                "not supported - Sorry!"))
            .arg(fileName).arg(fileHdr.versionMajor).arg(fileHdr.versionMinor);
    goto _exit;

_err_bad_magic:
    error = QString(tr("%1 is not a valid PCAP file")).arg(fileName);
    goto _exit;

_err_truncated:
    error = QString(tr("%1 is too short")).arg(fileName);
    goto _exit;

_err_open:
    error = QString(tr("Unable to open file: %1")).arg(fileName);
    goto _exit;

_exit:
    return isOk;
}

bool PcapFileFormat::saveStreams(const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    bool isOk = false;
    QFile file(fileName);
    QDataStream fd;
    PcapFileHeader fileHdr;
    PcapPacketHeader pktHdr;
    QByteArray pktBuf;

    if (!file.open(QIODevice::WriteOnly))
        goto _err_open;

    fd.setDevice(&file);

    fileHdr.magicNumber = kPcapFileMagic;
    fileHdr.versionMajor = kPcapFileVersionMajor;
    fileHdr.versionMinor = kPcapFileVersionMinor;
    fileHdr.thisZone = 0;
    fileHdr.sigfigs = 0;
    fileHdr.snapLen = kMaxSnapLen;
    fileHdr.network = 1; // Ethernet; FIXME: Hardcoding

    fd << fileHdr.magicNumber;
    fd << fileHdr.versionMajor;
    fd << fileHdr.versionMinor;
    fd << fileHdr.thisZone;
    fd << fileHdr.sigfigs;
    fd << fileHdr.snapLen;
    fd << fileHdr.network;

    pktBuf.resize(kMaxSnapLen);

    for (int i = 0; i < streams.stream_size(); i++)
    {
        StreamBase s;
        
        s.setId(i);
        s.protoDataCopyFrom(streams.stream(i));
        // TODO: expand frameIndex for each stream
        s.frameValue((uchar*)pktBuf.data(), pktBuf.size(), 0); 

        // TODO: write actual timing!?!?
        pktHdr.tsSec = 0;
        pktHdr.tsUsec = 0;
        pktHdr.inclLen = pktHdr.origLen = s.frameLen() - 4; // FCS; FIXME: Hardcoding
        if (pktHdr.inclLen > fileHdr.snapLen)
            pktHdr.inclLen = fileHdr.snapLen;

        fd << pktHdr.tsSec;
        fd << pktHdr.tsUsec;
        fd << pktHdr.inclLen;
        fd << pktHdr.origLen;
        fd.writeRawData(pktBuf.data(), pktHdr.inclLen);
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

