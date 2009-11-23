#include "userscript.h"

#include <qendian.h>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMetaObject>

UserScriptConfigForm::UserScriptConfigForm(UserScriptProtocol *protocol,
    QWidget *parent) : QWidget(parent), _protocol(protocol)
{
    setupUi(this);
}

void UserScriptConfigForm::on_programEdit_textChanged()
{
}

void UserScriptConfigForm::on_compileButton_clicked(bool checked)
{
    _protocol->storeConfigWidget();
    if (_protocol->userScriptErrorLineNumber() >= 0)
    {
        statusLabel->setText("<font color=\"red\">Fail</font>");
        QMessageBox::critical(this, "Error", 
            QString("Line %1: %2").arg(
                _protocol->userScriptErrorLineNumber()).arg(
                _protocol->userScriptErrorText()));
    }
    else
    {
        statusLabel->setText("<font color=\"green\">Success</font>");
    }
}

UserScriptProtocol::UserScriptProtocol(StreamBase *stream, AbstractProtocol *parent)
    : AbstractProtocol(stream, parent),
        _userProtocol(this)
{
    configForm = NULL;
    _isUpdated = false;
    _errorLineNumber = -1;

    _userProtocolScriptValue = _scriptEngine.newQObject(&_userProtocol);
    _userProtocolScriptValue.setProperty("protocolId", _scriptEngine.newObject());
    _scriptEngine.globalObject().setProperty("protocol", _userProtocolScriptValue);


    QScriptValue meta = _scriptEngine.newQMetaObject(_userProtocol.metaObject());
    _scriptEngine.globalObject().setProperty("Protocol", meta);

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
    return QString("%1:{UserScript}").arg(_userProtocol.name());
}

QString UserScriptProtocol::shortName() const
{
    return QString("%1:{Script}").arg(_userProtocol.shortName());
}

quint32 UserScriptProtocol::protocolId(ProtocolIdType type) const
{
    if (_userProtocol._protocolId.contains(static_cast<int>(type)))
        return _userProtocol._protocolId.value(static_cast<int>(type));
    else
        return AbstractProtocol::protocolId(type);
}

int    UserScriptProtocol::fieldCount() const
{
    return userScript_fieldCount;
}

QVariant UserScriptProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (index)
    {
        case userScript_program:
        {
            switch(attrib)
            {
                case FieldName:            
                    return QString("UserProtocol");

                case FieldValue:
                case FieldTextValue:
                    return QString().fromStdString(data.program());

                case FieldFrameValue:
                {
                    QScriptValue    userFunction;
                    QByteArray        fv;

                    evaluateUserScript();    // FIXME: don't call everytime!

                    if (_userProtocol.isProtocolFrameFixed())
                    {
                        fv = _userProtocol.protocolFrameFixedValue();
                        if (fv.size())
                            return fv;
                        else
                            return QByteArray(4, 0);    // FIXME
                    }

                    userFunction = _userProtocolScriptValue.property(
                            "protocolFrameValue");

                    qDebug("userscript protoFrameVal(): isValid:%d/isFunc:%d",
                        userFunction.isValid(), userFunction.isFunction());

                    if (userFunction.isValid() && userFunction.isFunction())
                    {
                        QScriptValue    userValue;

                        userValue = userFunction.call(QScriptValue(),
                            QScriptValueList() 
                                << QScriptValue(&_scriptEngine, streamIndex));

                        qDebug("userscript value: isValid:%d/isArray:%d",
                            userValue.isValid(), userValue.isArray());

                        if (userValue.isArray())
                        {
                            QList<int> pktBuf;
                           
                            qScriptValueToSequence(userValue, pktBuf); 

                            fv.resize(pktBuf.size());
                            for (int i = 0; i < pktBuf.size(); i++)
                                fv[i] = pktBuf.at(i) & 0xFF;
                        }
                    }

                    if (fv.size())
                        return fv;
                    else
                        return QByteArray(4, 0);    // FIXME
                }
                //case FieldBitSize:
                    //return 3;
                default:
                    break;
            }
            break;

        }
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

bool UserScriptProtocol::isProtocolFrameValueVariable() const
{
    return _userProtocol.isProtocolFrameValueVariable();
}

bool UserScriptProtocol::isProtocolFrameSizeVariable() const
{
    return _userProtocol.isProtocolFrameSizeVariable();
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

bool UserScriptProtocol::evaluateUserScript() const
{
    QScriptValue    userFunction;
    QScriptValue    userValue;

    _errorLineNumber = -1;
    _userProtocol.reset();
    _userProtocolScriptValue.setProperty("protocolFrameValue", QScriptValue());

    _scriptEngine.evaluate(
            fieldData(userScript_program, FieldValue).toString());
    if (_scriptEngine.hasUncaughtException())
        goto _error_exception;

    userFunction = _userProtocolScriptValue.property(
            "protocolFrameValue");
    qDebug("userscript eval protoFrameVal(): isValid:%d/isFunc:%d",
        userFunction.isValid(), userFunction.isFunction());
    if (!userFunction.isValid())
        goto _error_frame_value_not_set;

    if (userFunction.isArray())
    {
        QList<int> pktBuf;

        userValue = userFunction;
        _userProtocol.setProtocolFrameValueVariable(false);
       
        qScriptValueToSequence(userValue, pktBuf); 
        _userProtocol.setProtocolFrameFixedValue(pktBuf);
    }
    else if (userFunction.isFunction())
    {
        userValue = userFunction.call();
        if (_scriptEngine.hasUncaughtException())
            goto _error_exception;
        qDebug("userscript eval value: isValid:%d/isArray:%d",
            userValue.isValid(), userValue.isArray());
        if (!userValue.isArray())
            goto _error_not_an_array;

        if (_userProtocol.isProtocolFrameFixed())
        {
            QList<int> pktBuf;
           
            qScriptValueToSequence(userValue, pktBuf); 
            _userProtocol.setProtocolFrameFixedValue(pktBuf);
        }
    }
    else
        goto _error_frame_value_type;


    userValue = _userProtocolScriptValue.property("protocolId");

    if (userValue.isValid() && userValue.isObject())
    {
        QMetaEnum        meta;
        QScriptValue    userProtocolId;

        meta = _userProtocol.metaObject()->enumerator(
            _userProtocol.metaObject()->indexOfEnumerator("ProtocolIdType"));
        for (int i = 0; i < meta.keyCount(); i++)
        {
            userProtocolId = userValue.property(meta.key(i));

            qDebug("userscript protoId: isValid:%d/isNumber:%d",
                userProtocolId.isValid(),
                userProtocolId.isNumber());

            if (userProtocolId.isValid() && userProtocolId.isNumber())
                _userProtocol._protocolId.insert(
                        meta.value(i), userProtocolId.toUInt32());
        }
    }

    _isUpdated = true;
    return true;

_error_not_an_array:
    _errorLineNumber = userScriptLineCount();
    _errorText = QString("property 'protocolFrameValue' returns invalid array");
    goto _error;

_error_frame_value_type:
    _errorLineNumber = userScriptLineCount();
    _errorText = QString("property 'protocolFrameValue' is neither an array nor a function");
    goto _error;

_error_frame_value_not_set:
    _errorLineNumber = userScriptLineCount();
    _errorText = QString("mandatory property 'protocolFrameValue' is not set");
    goto _error;

_error_exception:
    _errorLineNumber = _scriptEngine.uncaughtExceptionLineNumber();
    _errorText = _scriptEngine.uncaughtException().toString();
    goto _error;

_error:
    _userProtocol.reset();
    return false;
}

int UserScriptProtocol::userScriptErrorLineNumber() const
{
    return _errorLineNumber;
}

QString UserScriptProtocol::userScriptErrorText() const
{
    return _errorText;
}

int UserScriptProtocol::userScriptLineCount() const
{
    return fieldData(userScript_program, FieldValue).toString().count(
            QChar('\n')) + 1;
}

//
// ---------------------------------------------------------------
//

UserProtocol::UserProtocol(AbstractProtocol *parent)
    : _parent (parent)
{
    reset();
}

bool UserProtocol::isProtocolFrameFixed() const
{
    return !isProtocolFrameValueVariable() && !isProtocolFrameSizeVariable();
}

const QByteArray& UserProtocol::protocolFrameFixedValue() const
{
    return _protocolFrameFixedValue;
}

void UserProtocol::setProtocolFrameFixedValue(const QByteArray& value)
{
    _protocolFrameFixedValue = value;
}

void UserProtocol::setProtocolFrameFixedValue(const QList<int>& value)
{
    _protocolFrameFixedValue.resize(value.size());
    for (int i = 0; i < value.size(); i++)
        _protocolFrameFixedValue[i] = value.at(i) & 0xFF;
}

void UserProtocol::reset()
{
    _name = QString();
    _shortName = QString();
    _protocolId.clear();
    _protocolFrameValueVariable = false;
    _protocolFrameSizeVariable = false;
    _protocolFrameFixedValue.resize(0);
}

QString UserProtocol::name() const
{
    return _name;
}

void UserProtocol::setName(QString &name)
{
    _name = name;
}

QString UserProtocol::shortName() const
{
    return _shortName;
}

void UserProtocol::setShortName(QString &shortName)
{
    _shortName = shortName;
}

bool UserProtocol::isProtocolFrameValueVariable() const
{
    return _protocolFrameValueVariable;
}

void UserProtocol::setProtocolFrameValueVariable(bool variable)
{
    qDebug("%s: %d", __FUNCTION__, variable);
    _protocolFrameValueVariable = variable;
}

bool UserProtocol::isProtocolFrameSizeVariable() const
{
    return _protocolFrameSizeVariable;
}

void UserProtocol::setProtocolFrameSizeVariable(bool variable)
{
    _protocolFrameSizeVariable = variable;
}

quint32 UserProtocol::payloadProtocolId(UserProtocol::ProtocolIdType type) const
{
    return _parent->payloadProtocolId(
            static_cast<AbstractProtocol::ProtocolIdType>(type));
}

int UserProtocol::protocolFrameOffset(int streamIndex) const
{
    return _parent->protocolFrameOffset(streamIndex);
}

int UserProtocol::protocolFramePayloadSize(int streamIndex) const
{
    return _parent->protocolFramePayloadSize(streamIndex);
}

bool UserProtocol::isProtocolFramePayloadValueVariable() const
{
    return _parent->isProtocolFramePayloadValueVariable();
}

bool UserProtocol::isProtocolFramePayloadSizeVariable() const
{
    return _parent->isProtocolFramePayloadSizeVariable();
}

quint32 UserProtocol::protocolFrameHeaderCksum(int streamIndex,
    AbstractProtocol::CksumType cksumType) const
{
    return _parent->protocolFrameHeaderCksum(streamIndex, cksumType);
}

quint32 UserProtocol::protocolFramePayloadCksum(int streamIndex,
    AbstractProtocol::CksumType cksumType) const
{
    return _parent->protocolFramePayloadCksum(streamIndex, cksumType);
}

/* vim: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab: */
