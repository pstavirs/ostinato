/*
Copyright (C) 2010, 2016 Srivats P.

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

#include "nativefileformat.h"

#include "crc32c.h"

#include <QCoreApplication>
#include <QFile>
#include <QVariant>

#define tr(str) QObject::tr(str)

const char* NativeFileFormat::kFileMagicValue = "\xa7\xb7OSTINATO";

static const int kBaseHex = 16;

static QString fileTypeStr(OstProto::FileType fileType)
{
    switch (fileType) {
        case OstProto::kReservedFileType:
            return QString("Reserved");
        case OstProto::kStreamsFileType:
            return QString("Streams");
        case OstProto::kSessionFileType:
            return QString("Streams");
        default:
            Q_ASSERT(false);
    }

    return QString("Unknown");
}

NativeFileFormat::NativeFileFormat()
{
    /*
     * We don't have any "real" work to do here in the constructor.
     * What we do is run some "assert" tests so that these get caught
     * at init itself instead of while saving/restoring when a user
     * might lose some data!
     */
    OstProto::FileMagic magic;
    OstProto::FileChecksum cksum;

    magic.set_value(kFileMagicValue);
    cksum.set_value(quint32(0));

    // TODO: convert Q_ASSERT to something that will run in RELEASE mode also
    Q_ASSERT(magic.IsInitialized());
    Q_ASSERT(cksum.IsInitialized());
    Q_ASSERT(magic.ByteSize() == kFileMagicSize);
    Q_ASSERT(cksum.ByteSize() == kFileChecksumSize);
}

bool NativeFileFormat::open(
        const QString fileName,
        OstProto::FileType fileType,
        OstProto::FileMeta &meta,
        OstProto::FileContent &content,
        QString &error)
{
    QFile file(fileName);
    QByteArray buf;
    int size, contentOffset, contentSize;
    quint32 calcCksum;
    OstProto::FileMagic magic;
    OstProto::FileChecksum cksum, zeroCksum;

    if (!file.open(QIODevice::ReadOnly))
        goto _open_fail;

    if (file.size() < kFileMagicSize)
        goto _magic_missing;

    if (file.size() < kFileMinSize)
        goto _checksum_missing;

    buf.resize(file.size());
    size = file.read(buf.data(), buf.size());
    if (size < 0)
        goto _read_fail;

    Q_ASSERT(file.atEnd());
    file.close();

    qDebug("%s: file.size() = %lld", __FUNCTION__, file.size());
    qDebug("%s: size = %d", __FUNCTION__, size);

    //qDebug("Read %d bytes", buf.size());
    //qDebug("%s", qPrintable(QString(buf.toHex())));

    // Parse and verify magic
    if (!magic.ParseFromArray(
                (void*)(buf.constData() + kFileMagicOffset),
                kFileMagicSize))
    {
        goto _magic_parse_fail;
    }
    if (magic.value() != kFileMagicValue)
        goto _magic_match_fail;

    // Parse and verify checksum
    if (!cksum.ParseFromArray(
            (void*)(buf.constData() + size - kFileChecksumSize),
            kFileChecksumSize))
    {
        goto _cksum_parse_fail;
    }

    zeroCksum.set_value(0);
    if (!zeroCksum.SerializeToArray(
                (void*) (buf.data() + size - kFileChecksumSize),
                kFileChecksumSize))
    {
        goto _zero_cksum_serialize_fail;
    }

    calcCksum = checksumCrc32C((quint8*) buf.constData(), size);

    qDebug("checksum \nExpected:%x Actual:%x",
        calcCksum, cksum.value());

    if (cksum.value() != calcCksum)
        goto _cksum_verify_fail;

    // Parse the metadata first before we parse the full contents
    if (!meta.ParseFromArray(
                (void*)(buf.constData() + kFileMetaDataOffset),
                fileMetaSize((quint8*)buf.constData(), size)))
    {
        goto _metadata_parse_fail;
    }

    qDebug("%s: File MetaData (INFORMATION) - \n%s", __FUNCTION__,
       meta.DebugString().c_str());
    qDebug("%s: END MetaData", __FUNCTION__);

    // MetaData Validation(s)
    if (meta.data().file_type() != fileType)
        goto _unexpected_file_type;

    if (meta.data().format_version_major() != kFileFormatVersionMajor)
        goto _incompatible_file_version;

    if (meta.data().format_version_minor() > kFileFormatVersionMinor)
        goto _incompatible_file_version;

    if (meta.data().format_version_minor() < kFileFormatVersionMinor)
    {
        // TODO: need to modify 'buf' such that we can parse successfully
        // assuming the native minor version
    }

    if (meta.data().format_version_revision() > kFileFormatVersionRevision)
    {
        error = QString(tr("%1 was created using a newer version of Ostinato."
           " New features/protocols will not be available.")).arg(fileName);
    }

    Q_ASSERT(meta.data().format_version_major() == kFileFormatVersionMajor);

    // ByteSize() does not include the Tag/Key, so we add 2 for that
    contentOffset = kFileMetaDataOffset + meta.data().ByteSize() + 2;
    contentSize = size - contentOffset - kFileChecksumSize;
    qDebug("%s: content offset/size = %d/%d", __FUNCTION__,
            contentOffset, contentSize);

    // Parse full contents
    if (!content.ParseFromArray(
            (void*)(buf.constData() + contentOffset),
            contentSize))
    {
        goto _content_parse_fail;
    }

    return true;

_content_parse_fail:
    error = QString(tr("Failed parsing %1 contents")).arg(fileName);
    qDebug("Error: %s", content.InitializationErrorString().c_str());
    qDebug("Debug: %s", content.DebugString().c_str());
    goto _fail;
_incompatible_file_version:
    error = QString(tr("%1 is in an incompatible format version - %2.%3.%4"
               " (Native version is %5.%6.%7)"))
            .arg(fileName)
            .arg(meta.data().format_version_major())
            .arg(meta.data().format_version_minor())
            .arg(meta.data().format_version_revision())
            .arg(kFileFormatVersionMajor)
            .arg(kFileFormatVersionMinor)
            .arg(kFileFormatVersionRevision);
    goto _fail;
_unexpected_file_type:
    error = QString(tr("%1 is not a %2 file"))
            .arg(fileName)
            .arg(fileTypeStr(fileType));
    goto _fail;
_metadata_parse_fail:
    error = QString(tr("Failed parsing %1 meta data")).arg(fileName);
    qDebug("Error: %s", meta.data().InitializationErrorString().c_str());
    goto _fail;
_cksum_verify_fail:
    error = QString(tr("%1 checksum validation failed!\nExpected:%2 Actual:%3"))
                .arg(fileName)
                .arg(calcCksum, 0, kBaseHex)
                .arg(cksum.value(), 0, kBaseHex);
    goto _fail;
_zero_cksum_serialize_fail:
    error = QString(tr("Internal Error: Zero Checksum Serialize failed!\n"
                "Error: %1\nDebug: %2"))
                .arg(QString().fromStdString(
                            cksum.InitializationErrorString()))
                .arg(QString().fromStdString(cksum.DebugString()));
    goto _fail;
_cksum_parse_fail:
    error = QString(tr("Failed parsing %1 checksum")).arg(fileName);
    qDebug("Error: %s", cksum.InitializationErrorString().c_str());
    goto _fail;
_magic_match_fail:
    error = QString(tr("%1 is not an Ostinato file")).arg(fileName);
    goto _fail;
_magic_parse_fail:
    error = QString(tr("%1 does not look like an Ostinato file")).arg(fileName);
    qDebug("Error: %s", magic.InitializationErrorString().c_str());
    goto _fail;
_read_fail:
    error = QString(tr("Error reading from %1")).arg(fileName);
    goto _fail;
_checksum_missing:
    error = QString(tr("%1 is too small (missing checksum)")).arg(fileName);
    goto _fail;
_magic_missing:
    error = QString(tr("%1 is too small (missing magic value)"))
                .arg(fileName);
    goto _fail;
_open_fail:
    error = QString(tr("Error opening %1")).arg(fileName);
    goto _fail;
_fail:
    qDebug("%s", qPrintable(error));
    return false;
}

bool NativeFileFormat::save(
        OstProto::FileType fileType,
        const OstProto::FileContent &content,
        const QString fileName,
        QString &error)
{
    OstProto::FileMagic magic;
    OstProto::FileMeta meta;
    OstProto::FileChecksum cksum;
    QFile file(fileName);
    int metaSize, contentSize;
    int contentOffset, cksumOffset;
    QByteArray buf;
    quint32 calcCksum;

    magic.set_value(kFileMagicValue);
    Q_ASSERT(magic.IsInitialized());

    cksum.set_value(0);
    Q_ASSERT(cksum.IsInitialized());

    initFileMetaData(*(meta.mutable_data()));
    meta.mutable_data()->set_file_type(fileType);
    Q_ASSERT(meta.IsInitialized());

    if (!content.IsInitialized())
        goto _content_not_init;

    Q_ASSERT(content.IsInitialized());

    metaSize = meta.ByteSize();
    contentSize = content.ByteSize();
    contentOffset = kFileMetaDataOffset + metaSize;
    cksumOffset = contentOffset + contentSize;

    Q_ASSERT(magic.ByteSize() == kFileMagicSize);
    Q_ASSERT(cksum.ByteSize() == kFileChecksumSize);
    buf.resize(kFileMagicSize + metaSize + contentSize + kFileChecksumSize);

    // Serialize everything
    if (!magic.SerializeToArray((void*) (buf.data() + kFileMagicOffset),
                kFileMagicSize))
    {
        goto _magic_serialize_fail;
    }

    if (!meta.SerializeToArray((void*) (buf.data() + kFileMetaDataOffset),
                metaSize))
    {
        goto _meta_serialize_fail;
    }

    if (!content.SerializeToArray((void*) (buf.data() + contentOffset),
                contentSize))
    {
        goto _content_serialize_fail;
    }

    if (!cksum.SerializeToArray((void*) (buf.data() + cksumOffset),
                kFileChecksumSize))
    {
        goto _zero_cksum_serialize_fail;
    }

    // TODO: emit status("Calculating checksum...");

    // Calculate and write checksum
    calcCksum = checksumCrc32C((quint8*)buf.constData(), buf.size());
    cksum.set_value(calcCksum);
    if (!cksum.SerializeToArray(
                (void*) (buf.data() + cksumOffset),
                kFileChecksumSize))
    {
        goto _cksum_serialize_fail;
    }

    qDebug("Writing %d bytes", buf.size());
    //qDebug("%s", qPrintable(QString(buf.toHex())));

    // TODO: emit status("Writing to disk...");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        goto _open_fail;

    if (file.write(buf) < 0)
        goto _write_fail;

    file.close();

    return true;

_write_fail:
    error = QString(tr("Error writing to %1")).arg(fileName);
    goto _fail;
_open_fail:
    error = QString(tr("Error opening %1 (Error Code = %2)"))
        .arg(fileName)
        .arg(file.error());
    goto _fail;
_cksum_serialize_fail:
    error = QString(tr("Internal Error: Checksum Serialize failed\n%1\n%2"))
                .arg(QString().fromStdString(
                            cksum.InitializationErrorString()))
                .arg(QString().fromStdString(cksum.DebugString()));
    goto _fail;
_zero_cksum_serialize_fail:
    error = QString(tr("Internal Eror: Zero Checksum Serialize failed\n%1\n%2"))
                .arg(QString().fromStdString(
                            cksum.InitializationErrorString()))
                .arg(QString().fromStdString(cksum.DebugString()));
    goto _fail;
_content_serialize_fail:
    error = QString(tr("Internal Error: Content Serialize failed\n%1\n%2"))
                .arg(QString().fromStdString(
                            content.InitializationErrorString()))
                .arg(QString().fromStdString(content.DebugString()));
    goto _fail;
_meta_serialize_fail:
    error = QString(tr("Internal Error: Meta Data Serialize failed\n%1\n%2"))
                .arg(QString().fromStdString(
                            meta.InitializationErrorString()))
                .arg(QString().fromStdString(meta.DebugString()));
    goto _fail;
_magic_serialize_fail:
    error = QString(tr("Internal Error: Magic Serialize failed\n%1\n%2"))
                .arg(QString().fromStdString(
                            magic.InitializationErrorString()))
                .arg(QString().fromStdString(magic.DebugString()));
    goto _fail;
_content_not_init:
    error = QString(tr("Internal Error: Content not initialized\n%1\n%2"))
                .arg(QString().fromStdString(
                            content.InitializationErrorString()))
                .arg(QString().fromStdString(content.DebugString()));
    goto _fail;
_fail:
    qDebug("%s", qPrintable(error));
    return false;
}

bool NativeFileFormat::isNativeFileFormat(
        const QString fileName,
        OstProto::FileType fileType)
{
    bool ret = false;
    QFile file(fileName);
    QByteArray buf;
    OstProto::FileMagic magic;

    if (!file.open(QIODevice::ReadOnly))
        goto _exit;

    // Assume tag/length for MetaData will fit in 8 bytes
    buf = file.peek(kFileMagicOffset + kFileMagicSize + 8);
    if (!magic.ParseFromArray((void*)(buf.constData() + kFileMagicOffset),
                kFileMagicSize))
        goto _close_exit;

    if (magic.value() == kFileMagicValue) {
        OstProto::FileMeta meta;
        int metaSize = fileMetaSize((quint8*)buf.constData(), buf.size());
        buf = file.peek(kFileMagicOffset + kFileMagicSize + metaSize);
        if (!meta.ParseFromArray(
                (void*)(buf.constData() + kFileMetaDataOffset), metaSize)) {
            qDebug("%s: File MetaData\n%s", __FUNCTION__,
                    meta.DebugString().c_str());
            goto _close_exit;
        }
        if (meta.data().file_type() == fileType)
            ret = true;
    }

_close_exit:
    file.close();
_exit:
    return ret;
}

void NativeFileFormat::initFileMetaData(OstProto::FileMetaData &metaData)
{
    QCoreApplication *app = QCoreApplication::instance();

    // Fill in the "native" file format version
    metaData.set_format_version_major(kFileFormatVersionMajor);
    metaData.set_format_version_minor(kFileFormatVersionMinor);
    metaData.set_format_version_revision(kFileFormatVersionRevision);

    metaData.set_generator_name(
        app->applicationName().toUtf8().constData());
    metaData.set_generator_version(
        app->property("version").toString().toUtf8().constData());
    metaData.set_generator_revision(
        app->property("revision").toString().toUtf8().constData());
}

int NativeFileFormat::fileMetaSize(const quint8* file, int size)
{
    int i = kFileMetaDataOffset;
    uint result, shift;
    const int kWireTypeLengthDelimited = 2;

    // An embedded Message field is encoded as
    // <Key> <Length> <Serialized-Value>
    // See Protobuf Encoding for more details

    // Decode 'Key' varint
    result = 0;
    shift = 0;
    while (i < size) {
      quint8 byte = file[i++];
      result |= (byte & 0x7f) << shift;
      if (!(byte & 0x80)) // MSB == 0?
        break;
      shift += 7;
    }

    if (i >= size)
        return 0;

    Q_ASSERT(result == ((OstProto::File::kMetaDataFieldNumber << 3)
                            | kWireTypeLengthDelimited));

    // Decode 'Length' varint
    result = 0;
    shift = 0;
    while (i < size) {
      quint8 byte = file[i++];
      result |= (byte & 0x7f) << shift;
      if (!(byte & 0x80)) // MSB == 0?
        break;
      shift += 7;
    }

    if (i >= size)
        return 0;

    return int(result+(i-kFileMetaDataOffset));
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/*! Fixup content to what is expected in the native version */
void NativeFileFormat::postParseFixup(OstProto::FileMetaData metaData,
        OstProto::FileContent &content)
{
    Q_ASSERT(metaData.format_version_major() == kFileFormatVersionMajor);

    // Do fixups from oldest to newest versions
    switch (metaData.format_version_minor())
    {
    case 1:
    {
        int n = content.matter().streams().stream_size();
        for (int i = 0; i < n; i++)
        {
            OstProto::StreamControl *sctl =
                content.mutable_matter()->mutable_streams()->mutable_stream(i)->mutable_control();
            sctl->set_packets_per_sec(sctl->obsolete_packets_per_sec());
            sctl->set_bursts_per_sec(sctl->obsolete_bursts_per_sec());
        }

        // fall-through to next higher version until native version
    }
    case kFileFormatVersionMinor: // native version
        break;

    case 0:
    default:
        qWarning("%s: minor version %u unhandled", __FUNCTION__,
                metaData.format_version_minor());
        Q_ASSERT_X(false, "postParseFixup", "unhandled minor version");
    }

}
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

