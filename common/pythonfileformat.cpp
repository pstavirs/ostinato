/*
Copyright (C) 2015 Srivats P.

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

#include "pythonfileformat.h"

#include <google/protobuf/descriptor.h>

#include <QFile>
#include <QSet>

#include <cctype>
#include <vector>

using google::protobuf::Message;
using google::protobuf::Reflection;
using google::protobuf::FieldDescriptor;

PythonFileFormat pythonFileFormat;

extern char *version;
extern char *revision;

PythonFileFormat::PythonFileFormat()
{
    // Nothing to do
}

PythonFileFormat::~PythonFileFormat()
{
    // Nothing to do
}

bool PythonFileFormat::open(const QString /*fileName*/,
        OstProto::StreamConfigList &/*streams*/, QString &/*error*/)
{
    // NOT SUPPORTED!
    return false;
}

bool PythonFileFormat::save(const OstProto::StreamConfigList streams,
        const QString fileName, QString &error)
{
    QFile file(fileName);
    QTextStream out(&file);
    QSet<QString> imports;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        goto _open_fail;

    // import standard modules
    emit status("Writing imports ...");
    emit target(0);
    writeStandardImports(out);

    emit target(streams.stream_size());
    // import protocols from respective modules
    // build the import list using a QSet to eliminate duplicates
    for (int i = 0; i < streams.stream_size(); i++) {
        const OstProto::Stream &stream = streams.stream(i);
        for (int j = 0 ; j < stream.protocol_size(); j++) {
            const OstProto::Protocol &protocol = stream.protocol(j);
            const Reflection *refl = protocol.GetReflection();
            std::vector<const FieldDescriptor*> fields;

            refl->ListFields(protocol, &fields);
            for (uint k = 0; k < fields.size(); k++) {
                // skip non extension fields
                if (!fields.at(k)->is_extension())
                    continue;

                if (fields.at(k)->file()->name() !=
                         fields.at(k)->message_type()->file()->name()) {
                    imports.insert(
                            QString("%1 import %2").arg(
                                QString(fields.at(k)->message_type()
                                        ->file()->name().c_str())
                                    .replace(".proto", "_pb2"),
                                fields.at(k)->message_type()->name().c_str()));
                    imports.insert(
                            QString("%1 import %2").arg(
                                QString(fields.at(k)
                                        ->file()->name().c_str())
                                    .replace(".proto", "_pb2"),
                                fields.at(k)->name().c_str()));
                }
                else {
                    imports.insert(
                            QString("%1 import %2, %3").arg(
                                QString(fields.at(k)->file()->name().c_str())
                                .replace(".proto", "_pb2"),
                                fields.at(k)->message_type()->name().c_str(),
                                fields.at(k)->name().c_str()));
                }
            }
        }
        emit progress(i);
    }
    // write the import statements
    out << "# import ostinato modules\n";
    out << "from ostinato.core import DroneProxy, ost_pb\n";
    foreach (QString str, imports)
        out << "from ostinato.protocols." << str << "\n";
    out << "\n";

    // start of script - init, connect to drone etc.
    emit status("Writing prologue ...");
    emit target(0);
    writePrologue(out);

    // Add streams
    emit status("Writing stream adds ...");
    emit target(streams.stream_size());
    out << "    # ------------#\n";
    out << "    # add streams #\n";
    out << "    # ------------#\n";
    out << "    stream_id = ost_pb.StreamIdList()\n";
    out << "    stream_id.port_id.id = tx_port_number\n";
    for (int i = 0; i < streams.stream_size(); i++) {
        out << "    stream_id.stream_id.add().id = " 
            << streams.stream(i).stream_id().id() << "\n";
        emit progress(i);
    }
    out << "    drone.addStream(stream_id)\n";
    out << "\n";

    // Configure streams with actual values
    emit status("Writing stream configuration ...");
    emit target(streams.stream_size());
    out << "    # ------------------#\n";
    out << "    # configure streams #\n";
    out << "    # ------------------#\n";
    out << "    stream_cfg = ost_pb.StreamConfigList()\n";
    out << "    stream_cfg.port_id.id = tx_port_number\n";
    for (int i = 0; i < streams.stream_size(); i++) {
        const OstProto::Stream &stream = streams.stream(i);
        const Reflection *refl;
        std::vector<const FieldDescriptor*> fields;

        out << "\n";
        out << "    # stream " << stream.stream_id().id() << " " 
            << stream.core().name().c_str() << "\n";
        out << "    s = stream_cfg.stream.add()\n";
        out << "    s.stream_id.id = " 
            << stream.stream_id().id() << "\n";

        // Stream Core values
        refl = stream.core().GetReflection();
        refl->ListFields(stream.core(), &fields);
        for (uint j = 0; j < fields.size(); j++) {
            writeFieldAssignment(out, QString("    s.core.")
                                        .append(fields.at(j)->name().c_str()),
                    stream.core(), refl, fields.at(j));
        }
         
        // Stream Control values
        refl = stream.control().GetReflection();
        refl->ListFields(stream.control(), &fields);
        for (uint j = 0; j < fields.size(); j++) {
            writeFieldAssignment(out, QString("    s.control.")
                                        .append(fields.at(j)->name().c_str()),
                    stream.control(), refl, fields.at(j));
        }

        // Protocols
        for (int j = 0 ; j < stream.protocol_size(); j++) {
            const OstProto::Protocol &protocol = stream.protocol(j);

            out << "\n"
                << "    p = s.protocol.add()\n"
                << "    p.protocol_id.id = "
                << QString(OstProto::Protocol_k_descriptor()
                            ->FindValueByNumber(protocol.protocol_id().id())
                                ->full_name().c_str())
                        .replace("OstProto", "ost_pb");
            out << "\n";
            refl = protocol.GetReflection();
            refl->ListFields(protocol, &fields);

            for (uint k = 0; k < fields.size(); k++) {
                // skip protocol_id field
                if (fields.at(k)->number() == 
                        OstProto::Protocol::kProtocolIdFieldNumber)
                    continue;
                QString pfx("    p.Extensions[X]");
                pfx.replace(fields.at(k)->is_extension()? "X": "Extensions[X]",
                        fields.at(k)->name().c_str());
                writeFieldAssignment(out, pfx, protocol,
                        refl, fields.at(k));
            }
        }
        emit progress(i);
    }
    out << "\n";
    out << "    drone.modifyStream(stream_cfg)\n";

    // end of script - transmit streams, disconnect from drone etc.
    emit status("Writing epilogue ...");
    emit target(0);
    writeEpilogue(out);

    out.flush();
    file.close();
    return true;

_open_fail:
    error = QString(tr("Error opening %1 (Error Code = %2)"))
        .arg(fileName)
        .arg(file.error());
    return false;
}

bool PythonFileFormat::isMyFileFormat(const QString /*fileName*/)
{
    // isMyFileFormat() is used for file open case to detect
    // file format - Open not supported for Python Scripts
    return false;
}

bool PythonFileFormat::isMyFileType(const QString fileType)
{
    if (fileType.startsWith("PythonScript"))
        return true;
    else
        return false;
}

//
// Private Member Functions
//
void PythonFileFormat::writeStandardImports(QTextStream &out)
{
    out << "#! /usr/bin/env python\n";
    out << "\n";
    out << "# This script was programmatically generated\n"
        << "# by Ostinato version " << version 
        << " revision " << revision << "\n"
        << "# The script should work out of the box mostly,\n"
        << "# but occassionally might need minor tweaking\n"
        << "# Please report any bugs at http://ostinato.org\n";
    out << "\n";
    out << "# standard modules\n";
    out << "import logging\n";
    out << "import os\n";
    out << "import sys\n";
    out << "import time\n";
    out << "\n";
}

void PythonFileFormat::writePrologue(QTextStream &out)
{
    out << "# initialize the below variables appropriately "
        << "to avoid manual input\n";
    out << "host_name = ''\n";
    out << "tx_port_number = -1\n";
    out << "\n";
    out << "# setup logging\n";
    out << "log = logging.getLogger(__name__)\n";
    out << "logging.basicConfig(level=logging.INFO)\n";
    out << "\n";
    out << "# get inputs, if required\n";
    out << "while len(host_name) == 0:\n";
    out << "    host_name = raw_input('Drone\\'s Hostname/IP: ')\n";
    out << "while tx_port_number < 0:\n";
    out << "    tx_port_number = int(raw_input('Tx Port Number: '))\n";
    out << "\n";
    out << "drone = DroneProxy(host_name)\n";
    out << "\n";
    out << "try:\n";
    out << "    # connect to drone\n";
    out << "    log.info('connecting to drone(%s:%d)' \n";
    out << "            % (drone.hostName(), drone.portNumber()))\n";
    out << "    drone.connect()\n";
    out << "\n";
    out << "    # setup tx port list\n";
    out << "    tx_port = ost_pb.PortIdList()\n";
    out << "    tx_port.port_id.add().id = tx_port_number;\n";
    out << "\n";
}

void PythonFileFormat::writeEpilogue(QTextStream &out)
{
    out << "    # clear tx/rx stats\n";
    out << "    log.info('clearing tx stats')\n";
    out << "    drone.clearStats(tx_port)\n";
    out << "\n";
    out << "    log.info('starting transmit')\n";
    out << "    drone.startTransmit(tx_port)\n";
    out << "\n";
    out << "    # wait for transmit to finish\n";
    out << "    log.info('waiting for transmit to finish ...')\n";
    out << "    while True:\n";
    out << "        try:\n";
    out << "            time.sleep(5)\n";
    out << "            tx_stats = drone.getStats(tx_port)\n";
    out << "            if tx_stats.port_stats[0].state.is_transmit_on"
                                " == False:\n";
    out << "                break\n";
    out << "        except KeyboardInterrupt:\n";
    out << "            log.info('transmit interrupted by user')\n";
    out << "            break\n";
    out << "\n";
    out << "    # stop transmit and capture\n";
    out << "    log.info('stopping transmit')\n";
    out << "    drone.stopTransmit(tx_port)\n";
    out << "\n";
    out << "    # get tx stats\n";
    out << "    log.info('retreiving stats')\n";
    out << "    tx_stats = drone.getStats(tx_port)\n";
    out << "\n";
    out << "    log.info('tx pkts = %d' % (tx_stats.port_stats[0].tx_pkts))\n";
    out << "\n";
    out << "    # delete streams\n";
    out << "    log.info('deleting tx_streams')\n";
    out << "    drone.deleteStream(stream_id)\n";
    out << "\n";
    out << "    # bye for now\n";
    out << "    drone.disconnect()\n";
    out << "\n";
    out << "except Exception as ex:\n";
    out << "    log.exception(ex)\n";
    out << "    sys.exit(1)\n";
}

void PythonFileFormat::writeFieldAssignment(
        QTextStream &out, 
        QString fieldName,
        const Message &msg, 
        const Reflection *refl, 
        const FieldDescriptor *fieldDesc,
        int index)
{
    // for a repeated field,
    //     if index < 0  => we are writing a repeated aggregate
    //     if index >= 0 => we are writing a repeated element 
    if (fieldDesc->is_repeated() && (index < 0)) {
        int n = refl->FieldSize(msg, fieldDesc);
        QString var = singularize(fieldDesc->name().c_str());
        for (int i = 0; i < n; i++) {
            out << "    " << var << " = " << fieldName.trimmed() << ".add()\n";
            writeFieldAssignment(out, QString("    ").append(var), 
                                 msg, refl, fieldDesc, i);
        }
        return;
    }

    // Ideally fields should not be set if they have the same
    // value as the default value - but currently protocols don't
    // check this when setting values in the protobuf data object
    // so here we check that explicitly for each field and if true
    // we don't output anything
    switch(fieldDesc->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
        {
            qint32 val = fieldDesc->is_repeated() ?
                refl->GetRepeatedInt32(msg, fieldDesc, index) :
                refl->GetInt32(msg, fieldDesc);
            if (val != fieldDesc->default_value_int32())
                out << fieldName << " = " << val << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_INT64:
        {
            qint64 val = fieldDesc->is_repeated() ?
                refl->GetRepeatedInt64(msg, fieldDesc, index) :
                refl->GetInt64(msg, fieldDesc);
            if (val != fieldDesc->default_value_int64())
                out << fieldName << " = " << val << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_UINT32:
        {
            quint32 val = fieldDesc->is_repeated() ?
                refl->GetRepeatedUInt32(msg, fieldDesc, index) :
                refl->GetUInt32(msg, fieldDesc);
            QString valStr;

            if (useDecimalBase(fieldName))
                valStr.setNum(val);
            else
                valStr.setNum(val, 16).prepend("0x");

            if (val != fieldDesc->default_value_uint32())
                out << fieldName << " = " << valStr << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_UINT64:
        {
            quint64 val = fieldDesc->is_repeated() ?
                refl->GetRepeatedUInt64(msg, fieldDesc, index) :
                refl->GetUInt64(msg, fieldDesc);
            QString valStr;

            if (useDecimalBase(fieldName))
                valStr.setNum(val);
            else
                valStr.setNum(val, 16).prepend("0x");

            if (val != fieldDesc->default_value_uint64())
                out << fieldName << " = " << valStr << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_DOUBLE:
        {
            double val = fieldDesc->is_repeated() ?
                refl->GetRepeatedDouble(msg, fieldDesc, index) :
                refl->GetDouble(msg, fieldDesc);
            if (val != fieldDesc->default_value_double())
                out << fieldName << " = " << val << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_FLOAT:
        {
            float val = fieldDesc->is_repeated() ?
                refl->GetRepeatedFloat(msg, fieldDesc, index) :
                refl->GetFloat(msg, fieldDesc);
            if (val != fieldDesc->default_value_float())
                out << fieldName << " = " << val << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_BOOL:
        {
            bool val = fieldDesc->is_repeated() ?
                refl->GetRepeatedBool(msg, fieldDesc, index) :
                refl->GetBool(msg, fieldDesc);
            if (val != fieldDesc->default_value_bool())
                out << fieldName 
                    << " = " 
                    << (refl->GetBool(msg, fieldDesc) ? "True" : "False")
                    << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_STRING:
        {
            std::string val = fieldDesc->is_repeated() ?
                refl->GetRepeatedStringReference(msg, fieldDesc, index, &val) :
                refl->GetStringReference(msg, fieldDesc, &val);
            QString escVal = escapeString(QString::fromStdString(val));
            if (val != fieldDesc->default_value_string())
                out << fieldName << " = '" << escVal << "'\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_ENUM:
        {
            // Fields defined in protocol.proto are within ost_pb scope
            QString module = fieldDesc->file()->name() == "protocol.proto" ? 
                                "ost_pb." : "";
            std::string val = fieldDesc->is_repeated() ?
                refl->GetRepeatedEnum(msg, fieldDesc, index)->full_name() :
                refl->GetEnum(msg, fieldDesc)->full_name();
            if (val != fieldDesc->default_value_enum()->full_name())
                out << fieldName << " = " << QString::fromStdString(val)
                                                 .replace("OstProto.", module)
                    << "\n";
            break;
        }
        case FieldDescriptor::CPPTYPE_MESSAGE: 
        {
            QString pfxStr(fieldName);
            const Message &msg2 = fieldDesc->is_repeated() ?
                refl->GetRepeatedMessage(msg, fieldDesc, index) :
                refl->GetMessage(msg, fieldDesc);
            const Reflection *refl2 = msg2.GetReflection();
            std::vector<const FieldDescriptor*> fields2;
            QList<std::string> autoFields;

            refl2->ListFields(msg2, &fields2);

            // Unfortunately, auto-calculated fields such as cksum, length 
            // and protocol-type etc. may be set in the protobuf even if
            // they are not being overridden;
            // Intelligence regarding them is inside the respective protocol 
            // implementation, not inside the protobuf objects - the latter
            // is all we have available here to work with;
            // We attempt a crude hack here to detect such fields and avoid
            // writing assignment statements for them
            for (uint i = 0; i < fields2.size(); i++) {
                std::string name = fields2.at(i)->name();
                if ((fields2.at(i)->cpp_type() 
                            == FieldDescriptor::CPPTYPE_BOOL)
                        && (name.find("is_override_") == 0)
                        && (refl2->GetBool(msg2, fields2.at(i)) == false)) {
                    name.erase(0, sizeof("is_override_") - 1);
                    autoFields.append(name);
                }
            }

            for (uint i = 0 ; i < fields2.size(); i++) {
                // skip auto fields that are not overridden
                if (autoFields.contains(fields2.at(i)->name()))
                    continue;

                writeFieldAssignment(out, 
                        QString("%1.%2").arg(pfxStr, 
                                             fields2.at(i)->name().c_str()),
                        msg2, refl2, fields2.at(i));
            }
            break;
        }
        default:
            qWarning("unable to write field of unsupported type %d",
                    fieldDesc->cpp_type());
    }
}

QString PythonFileFormat::singularize(QString plural)
{
    QString singular = plural;

    // Apply some heuristics
    if (plural.endsWith("ies"))
        singular.replace(singular.length()-3, 3, "y");
    else if (plural.endsWith("ses"))
        singular.chop(2);
    else if (plural.endsWith("s"))
        singular.chop(1);

    return singular;
}

QString PythonFileFormat::escapeString(QString str)
{
    QString escStr = "";
    for (int i=0; i < str.length(); i++) {
        uchar c = str[i].cell();
        if ((c < 128) && isprint(c)) {
            if (c == '\'')
                escStr.append("\\'");
            else
                escStr.append(str[i]);
        }
        else
            escStr.append(QString("\\x%1").arg(int(c), 2, 16, QChar('0')));
    }
    return escStr;
}

bool PythonFileFormat::useDecimalBase(QString fieldName)
{
    // Heuristic - use Hex base for all except for the following
    return fieldName.endsWith("count") 
            || fieldName.endsWith("length")
            || fieldName.endsWith("len")
            || fieldName.endsWith("time");
}

