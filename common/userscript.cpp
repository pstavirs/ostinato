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

#include "userscript.h"

#include <QMessageBox>

//
// -------------------- UserScriptConfigForm --------------------
//

UserScriptConfigForm::UserScriptConfigForm(UserScriptProtocol *protocol,
    QWidget *parent) : QWidget(parent), protocol_(protocol)
{
    setupUi(this);
    updateStatus();
}

void UserScriptConfigForm::updateStatus()
{
    if (protocol_->isScriptValid())
    {
        statusLabel->setText(QString("<font color=\"green\">Success</font>"));
        compileButton->setDisabled(true);
    }
    else
    {
        statusLabel->setText(
                QString("<font color=\"red\">Error: %1: %2</font>").arg(
                protocol_->userScriptErrorLineNumber()).arg(
                protocol_->userScriptErrorText()));
        compileButton->setEnabled(true);
    }
}

void UserScriptConfigForm::on_programEdit_textChanged()
{
    compileButton->setEnabled(true);
}

void UserScriptConfigForm::on_compileButton_clicked(bool /*checked*/)
{
    protocol_->storeConfigWidget();
    if (!protocol_->isScriptValid())
    {
        QMessageBox::critical(this, "Error", 
            QString("%1: %2").arg(
                protocol_->userScriptErrorLineNumber()).arg(
                protocol_->userScriptErrorText()));
    }
    updateStatus();
}

//
// -------------------- UserScriptProtocol --------------------
//

UserScriptProtocol::UserScriptProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent),
        userProtocol_(this)
{
    configForm = NULL;
    isScriptValid_ = false;
    errorLineNumber_ = 0;

    userProtocolScriptValue_ = engine_.newQObject(&userProtocol_);
    engine_.globalObject().setProperty("protocol", userProtocolScriptValue_);

    QScriptValue meta = engine_.newQMetaObject(userProtocol_.metaObject());
    engine_.globalObject().setProperty("Protocol", meta);
}

UserScriptProtocol::~UserScriptProtocol()
{
    delete configForm;
}

AbstractProtocol* UserScriptProtocol::createInstance(StreamBase *stream,
    AbstractProtocol *parent)
{
    return new UserScriptProtocol(stream, parent);
}

quint32 UserScriptProtocol::protocolNumber() const
{
    return OstProto::Protocol::kUserScriptFieldNumber;
}

void UserScriptProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
    protocol.MutableExtension(OstProto::userScript)->CopyFrom(data);
    protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void UserScriptProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
    if (protocol.protocol_id().id() == protocolNumber() &&
            protocol.HasExtension(OstProto::userScript))
        data.MergeFrom(protocol.GetExtension(OstProto::userScript));

    evaluateUserScript();
}

QString UserScriptProtocol::name() const
{
    return QString("%1:{UserScript} [EXPERIMENTAL]").arg(userProtocol_.name());
}

QString UserScriptProtocol::shortName() const
{
    return QString("%1:{Script} [EXPERIMENTAL]").arg(userProtocol_.name());
}

quint32 UserScriptProtocol::protocolId(ProtocolIdType type) const
{
    QScriptValue userFunction;
    QScriptValue userValue;

    if (!isScriptValid_)
        goto _do_default;

    userFunction = userProtocolScriptValue_.property("protocolId");

    if (!userFunction.isValid())
        goto _do_default;

    Q_ASSERT(userFunction.isFunction());

    userValue = userFunction.call(QScriptValue(),
        QScriptValueList() << QScriptValue(&engine_, type));

    Q_ASSERT(userValue.isValid());
    Q_ASSERT(userValue.isNumber());

    return userValue.toUInt32();

_do_default:
    return AbstractProtocol::protocolId(type);
}

int UserScriptProtocol::fieldCount() const
{
    return userScript_fieldCount;
}

QVariant UserScriptProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
    case userScript_program:

        switch(attrib)
        {
        case FieldName:            
            return QString("UserProtocol");

        case FieldValue:
        case FieldTextValue:
            return QString().fromStdString(data.program());

        case FieldFrameValue:
        {
            if (!isScriptValid_)
                return QByteArray();

            QScriptValue userFunction = userProtocolScriptValue_.property(
                    "protocolFrameValue");

            Q_ASSERT(userFunction.isValid());
            Q_ASSERT(userFunction.isFunction());

            QScriptValue userValue = userFunction.call(QScriptValue(),
                QScriptValueList() << QScriptValue(&engine_, streamIndex));

            Q_ASSERT(userValue.isValid());
            Q_ASSERT(userValue.isArray());

            QByteArray fv;
            QList<int> pktBuf;
               
            qScriptValueToSequence(userValue, pktBuf); 

            fv.resize(pktBuf.size());
            for (int i = 0; i < pktBuf.size(); i++)
                fv[i] = pktBuf.at(i) & 0xFF;

            return fv;
        }
        default:
            break;
        }
        break;

    default:
        qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
            index);
        break;
    }

    return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool UserScriptProtocol::setFieldData(int index, const QVariant &value, 
        FieldAttrib attrib)
{
    bool isOk = false;

    if (attrib != FieldValue)
        goto _exit;

    switch (index)
    {
        case userScript_program:
        {
            data.set_program(value.toString().toStdString());
            break;
        }
        default:
            qFatal("%s: unimplemented case %d in switch", __PRETTY_FUNCTION__,
                index);
            break;
    }

_exit:
    return isOk;
}

int UserScriptProtocol::protocolFrameSize(int streamIndex) const
{
    if (!isScriptValid_)
        return 0;

    QScriptValue userFunction = userProtocolScriptValue_.property(
            "protocolFrameSize");

    Q_ASSERT(userFunction.isValid());
    Q_ASSERT(userFunction.isFunction());

    QScriptValue userValue = userFunction.call(QScriptValue(), 
            QScriptValueList() << QScriptValue(&engine_, streamIndex));

    Q_ASSERT(userValue.isNumber());

    return userValue.toInt32();
}

bool UserScriptProtocol::isProtocolFrameValueVariable() const
{
    return userProtocol_.isProtocolFrameValueVariable();
}

bool UserScriptProtocol::isProtocolFrameSizeVariable() const
{
    return userProtocol_.isProtocolFrameSizeVariable();
}

int UserScriptProtocol::protocolFrameVariableCount() const
{
    return userProtocol_.protocolFrameVariableCount();
}

quint32 UserScriptProtocol::protocolFrameCksum(int streamIndex,
        CksumType cksumType) const
{
    QScriptValue userFunction;
    QScriptValue userValue;

    if (!isScriptValid_)
        goto _do_default;

    userFunction = userProtocolScriptValue_.property("protocolFrameCksum");

    qDebug("userscript protoFrameCksum(): isValid:%d/isFunc:%d",
        userFunction.isValid(), userFunction.isFunction());

    if (!userFunction.isValid())
        goto _do_default;

    Q_ASSERT(userFunction.isFunction());

    userValue = userFunction.call(QScriptValue(),
            QScriptValueList() << QScriptValue(&engine_, streamIndex)
            << QScriptValue(&engine_, cksumType));

    Q_ASSERT(userValue.isValid());
    Q_ASSERT(userValue.isNumber());

    return userValue.toUInt32();

_do_default:
    return AbstractProtocol::protocolFrameCksum(streamIndex, cksumType);
}

QWidget* UserScriptProtocol::configWidget()
{
    if (configForm == NULL)
    {
        configForm = new UserScriptConfigForm(this);
        loadConfigWidget();
    }

    return configForm;
}

void UserScriptProtocol::loadConfigWidget()
{
    configWidget();

    configForm->programEdit->setPlainText(
        fieldData(userScript_program, FieldValue).toString());
}

void UserScriptProtocol::storeConfigWidget()
{
    configWidget();
    setFieldData(userScript_program, configForm->programEdit->toPlainText());
    evaluateUserScript();
}

void UserScriptProtocol::evaluateUserScript() const
{
    QScriptValue    userFunction;
    QScriptValue    userValue;
    QString         property;

    isScriptValid_ = false;
    errorLineNumber_ = userScriptLineCount();

    // Reset all properties including the dynamic ones
    userProtocol_.reset();
    userProtocolScriptValue_.setProperty("protocolFrameValue", QScriptValue());
    userProtocolScriptValue_.setProperty("protocolFrameSize", QScriptValue());
    userProtocolScriptValue_.setProperty("protocolFrameCksum", QScriptValue());
    userProtocolScriptValue_.setProperty("protocolId", QScriptValue());

    engine_.evaluate(fieldData(userScript_program, FieldValue).toString());
    if (engine_.hasUncaughtException())
        goto _error_exception;

    // Validate protocolFrameValue()
    property = QString("protocolFrameValue");
    userFunction = userProtocolScriptValue_.property(property);

    qDebug("userscript property %s: isValid:%d/isFunc:%d", 
            property.toAscii().constData(),
            userFunction.isValid(), userFunction.isFunction());

    if (!userFunction.isValid())
    {
        errorText_ = property + QString(" not set");
        goto _error_exit;
    }

    if (!userFunction.isFunction())
    {
        errorText_ = property + QString(" is not a function");
        goto _error_exit;
    }

    userValue = userFunction.call();
    if (engine_.hasUncaughtException())
        goto _error_exception;

    qDebug("userscript property %s return value: isValid:%d/isArray:%d",
            property.toAscii().constData(),
            userValue.isValid(), userValue.isArray());

    if (!userValue.isArray())
    {
        errorText_ = property + QString(" does not return an array");
        goto _error_exit;
    }

    // Validate protocolFrameSize()
    property = QString("protocolFrameSize");
    userFunction = userProtocolScriptValue_.property(property);

    qDebug("userscript property %s: isValid:%d/isFunc:%d", 
            property.toAscii().constData(),
            userFunction.isValid(), userFunction.isFunction());

    if (!userFunction.isValid())
    {
        errorText_ = property + QString(" not set");
        goto _error_exit;
    }

    if (!userFunction.isFunction())
    {
        errorText_ = property + QString(" is not a function");
        goto _error_exit;
    }

    userValue = userFunction.call();
    if (engine_.hasUncaughtException())
        goto _error_exception;

    qDebug("userscript property %s return value: isValid:%d/isNumber:%d",
            property.toAscii().constData(),
            userValue.isValid(), userValue.isNumber());

    if (!userValue.isNumber())
    {
        errorText_ = property + QString(" does not return a number");
        goto _error_exit;
    }

    // Validate protocolFrameCksum() [optional]
    property = QString("protocolFrameCksum");
    userFunction = userProtocolScriptValue_.property(property);

    qDebug("userscript property %s: isValid:%d/isFunc:%d", 
            property.toAscii().constData(),
            userFunction.isValid(), userFunction.isFunction());

    if (!userFunction.isValid())
        goto _skip_cksum;

    if (!userFunction.isFunction())
    {
        errorText_ = property + QString(" is not a function");
        goto _error_exit;
    }

    userValue = userFunction.call();
    if (engine_.hasUncaughtException())
        goto _error_exception;

    qDebug("userscript property %s return value: isValid:%d/isNumber:%d",
            property.toAscii().constData(),
            userValue.isValid(), userValue.isNumber());

    if (!userValue.isNumber())
    {
        errorText_ = property + QString(" does not return a number");
        goto _error_exit;
    }


_skip_cksum:
    // Validate protocolId() [optional]
    property = QString("protocolId");
    userFunction = userProtocolScriptValue_.property(property);

    qDebug("userscript property %s: isValid:%d/isFunc:%d", 
            property.toAscii().constData(),
            userFunction.isValid(), userFunction.isFunction());

    if (!userFunction.isValid())
        goto _skip_protocol_id;

    if (!userFunction.isFunction())
    {
        errorText_ = property + QString(" is not a function");
        goto _error_exit;
    }

    userValue = userFunction.call();
    if (engine_.hasUncaughtException())
        goto _error_exception;

    qDebug("userscript property %s return value: isValid:%d/isNumber:%d",
            property.toAscii().constData(),
            userValue.isValid(), userValue.isNumber());

    if (!userValue.isNumber())
    {
        errorText_ = property + QString(" does not return a number");
        goto _error_exit;
    }


_skip_protocol_id:
    errorText_ = QString("");
    isScriptValid_ = true;
    return;

_error_exception:
    errorLineNumber_ = engine_.uncaughtExceptionLineNumber();
    errorText_ = engine_.uncaughtException().toString();

_error_exit:
    userProtocol_.reset();
    return;
}

bool UserScriptProtocol::isScriptValid() const
{
    return isScriptValid_;
}

int UserScriptProtocol::userScriptErrorLineNumber() const
{
    return errorLineNumber_;
}

QString UserScriptProtocol::userScriptErrorText() const
{
    return errorText_;
}

int UserScriptProtocol::userScriptLineCount() const
{
    return fieldData(userScript_program, FieldValue).toString().count(
            QChar('\n')) + 1;
}

//
// -------------------- UserProtocol --------------------
//

UserProtocol::UserProtocol(AbstractProtocol *parent)
    : parent_ (parent)
{
    reset();
}

void UserProtocol::reset()
{
    name_ = QString();
    protocolFrameValueVariable_ = false;
    protocolFrameSizeVariable_ = false;
    protocolFrameVariableCount_ = 1;
}

QString UserProtocol::name() const
{
    return name_;
}

void UserProtocol::setName(QString &name)
{
    name_ = name;
}

bool UserProtocol::isProtocolFrameValueVariable() const
{
    return protocolFrameValueVariable_;
}

void UserProtocol::setProtocolFrameValueVariable(bool variable)
{
    protocolFrameValueVariable_ = variable;
}

bool UserProtocol::isProtocolFrameSizeVariable() const
{
    return protocolFrameSizeVariable_;
}

void UserProtocol::setProtocolFrameSizeVariable(bool variable)
{
    protocolFrameSizeVariable_ = variable;
}

int UserProtocol::protocolFrameVariableCount() const
{
    return protocolFrameVariableCount_;
}

void UserProtocol::setProtocolFrameVariableCount(int count)
{
    protocolFrameVariableCount_ = count;
}

quint32 UserProtocol::payloadProtocolId(UserProtocol::ProtocolIdType type) const
{
    return parent_->payloadProtocolId(
            static_cast<AbstractProtocol::ProtocolIdType>(type));
}

int UserProtocol::protocolFrameOffset(int streamIndex) const
{
    return parent_->protocolFrameOffset(streamIndex);
}

int UserProtocol::protocolFramePayloadSize(int streamIndex) const
{
    return parent_->protocolFramePayloadSize(streamIndex);
}

bool UserProtocol::isProtocolFramePayloadValueVariable() const
{
    return parent_->isProtocolFramePayloadValueVariable();
}

bool UserProtocol::isProtocolFramePayloadSizeVariable() const
{
    return parent_->isProtocolFramePayloadSizeVariable();
}

int UserProtocol::protocolFramePayloadVariableCount() const
{
    return parent_->protocolFramePayloadVariableCount();
}

quint32 UserProtocol::protocolFrameHeaderCksum(int streamIndex,
    AbstractProtocol::CksumType cksumType) const
{
    return parent_->protocolFrameHeaderCksum(streamIndex, cksumType);
}

quint32 UserProtocol::protocolFramePayloadCksum(int streamIndex,
    AbstractProtocol::CksumType cksumType) const
{
    quint32 cksum;

    cksum = parent_->protocolFramePayloadCksum(streamIndex, cksumType);
    qDebug("UserProto:%s = %d", __FUNCTION__, cksum);
    return cksum;
}

/* vim: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab: */
