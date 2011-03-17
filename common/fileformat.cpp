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

#include "fileformat.h"

#include "crc32c.h"

#include <QApplication>
#include <QFile>
#include <QVariant>

#include <string>

const std::string FileFormat::kFileMagicValue = "\xa7\xb7OSTINATO";

FileFormat fileFormat;

const int kBaseHex = 16;

FileFormat::FileFormat()
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

FileFormat::~FileFormat()
{
}

bool FileFormat::openStreams(const QString fileName, 
            OstProto::StreamConfigList &streams, QString &error)
{
    QFile file(fileName);
    QByteArray buf;
    int size, contentOffset, contentSize;
    quint32 calcCksum;
    OstProto::FileMagic magic;
    OstProto::FileMeta meta;
    OstProto::FileContent content;
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
    //qDebug("%s", QString(buf.toHex()).toAscii().constData());

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
                size - kFileMetaDataOffset))
    {
        goto _metadata_parse_fail;
    }

    qDebug("%s: File MetaData (INFORMATION) - \n%s", __FUNCTION__, 
       QString().fromStdString(meta.DebugString()).toAscii().constData());

    // MetaData Validation(s)
    if (meta.data().file_type() != OstProto::kStreamsFileType)
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
    Q_ASSERT(meta.data().format_version_minor() == kFileFormatVersionMinor);

    // ByteSize() does not include the Tag/Key, so we add 2 for that
    contentOffset = kFileMetaDataOffset + meta.data().ByteSize() + 2;
    contentSize = size - contentOffset - kFileChecksumSize;

    // Parse full contents
    if (!content.ParseFromArray(
            (void*)(buf.constData() + contentOffset), 
            contentSize))
    {
        goto _content_parse_fail;
    }

    if (!content.matter().has_streams())
        goto _missing_streams;

    streams.CopyFrom(content.matter().streams());

    return true;

_missing_streams:
    error = QString(tr("%1 does not contain any streams")).arg(fileName);
    goto _fail;
_content_parse_fail:
    error = QString(tr("Failed parsing %1 contents")).arg(fileName);
    qDebug("Error: %s", QString().fromStdString(
            content.matter().InitializationErrorString())
                .toAscii().constData());
    qDebug("Debug: %s", QString().fromStdString(
            content.matter().DebugString()).toAscii().constData());
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
    error = QString(tr("%1 is not a streams file")).arg(fileName);
    goto _fail;
_metadata_parse_fail:
    error = QString(tr("Failed parsing %1 meta data")).arg(fileName);
    qDebug("Error: %s", QString().fromStdString(
            meta.data().InitializationErrorString())
                .toAscii().constData());
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
    qDebug("Error: %s", QString().fromStdString(
            cksum.InitializationErrorString())
                .toAscii().constData());
    goto _fail;
_magic_match_fail:
    error = QString(tr("%1 is not an Ostinato file")).arg(fileName);
    goto _fail;
_magic_parse_fail:
    error = QString(tr("%1 does not look like an Ostinato file")).arg(fileName);
    qDebug("Error: %s", QString().fromStdString(
            magic.InitializationErrorString())
                .toAscii().constData());
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
    qDebug("%s", error.toAscii().constData());
    return false;
}

bool FileFormat::saveStreams(const OstProto::StreamConfigList streams, 
        const QString fileName, QString &error)
{
    OstProto::FileMagic magic;
    OstProto::FileMeta meta;
    OstProto::FileContent content;
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
    meta.mutable_data()->set_file_type(OstProto::kStreamsFileType);
    Q_ASSERT(meta.IsInitialized());

    if (!streams.IsInitialized())
        goto _stream_not_init;

    content.mutable_matter()->mutable_streams()->CopyFrom(streams);
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

    emit status("Calculating checksum...");

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
    //qDebug("%s", QString(buf.toHex()).toAscii().constData());

    emit status("Writing to disk...");
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
_stream_not_init:
    error = QString(tr("Internal Error: Streams not initialized\n%1\n%2"))
                .arg(QString().fromStdString(
                            streams.InitializationErrorString()))
                .arg(QString().fromStdString(streams.DebugString()));
    goto _fail;
_fail:
    qDebug("%s", error.toAscii().constData());
    return false;
}

bool FileFormat::isMyFileFormat(const QString fileName)
{
    bool ret = false;
    QFile file(fileName);
    QByteArray buf;
    OstProto::FileMagic magic;

    if (!file.open(QIODevice::ReadOnly))
        goto _exit;

    buf = file.peek(kFileMagicOffset + kFileMagicSize);
    if (!magic.ParseFromArray((void*)(buf.constData() + kFileMagicOffset), 
                kFileMagicSize))
        goto _close_exit;

    if (magic.value() == kFileMagicValue)
        ret = true;

_close_exit:
    file.close();
_exit:
    return ret;
}

bool FileFormat::isMyFileType(const QString fileType)
{
    if (fileType.startsWith("Ostinato"))
        return true;
    else
        return false;
}

void FileFormat::initFileMetaData(OstProto::FileMetaData &metaData)
{
    // Fill in the "native" file format version
    metaData.set_format_version_major(kFileFormatVersionMajor);
    metaData.set_format_version_minor(kFileFormatVersionMinor);
    metaData.set_format_version_revision(kFileFormatVersionRevision);

    metaData.set_generator_name(
        qApp->applicationName().toUtf8().constData());
    metaData.set_generator_version(
        qApp->property("version").toString().toUtf8().constData());
    metaData.set_generator_revision(
        qApp->property("revision").toString().toUtf8().constData());
}

