#ifndef _USER_SCRIPT_H
#define _USER_SCRIPT_H

#include "abstractprotocol.h"
#include "userscript.pb.h"
#include "ui_userscript.h"

#include <QScriptEngine>
#include <QScriptValue>

class UserScriptProtocol;

class UserProtocol : public QObject
{
    friend class UserScriptProtocol;

    Q_OBJECT;
    Q_ENUMS(ProtocolIdType);

    Q_PROPERTY(QString name READ name WRITE setName);
    Q_PROPERTY(QString shortName READ shortName WRITE setShortName);
    Q_PROPERTY(bool protocolFrameValueVariable
            READ isProtocolFrameValueVariable
            WRITE setProtocolFrameValueVariable);
    Q_PROPERTY(bool protocolFrameSizeVariable
            READ isProtocolFrameSizeVariable
            WRITE setProtocolFrameSizeVariable);
    
public:
    enum ProtocolIdType
    {
        ProtocolIdLlc = AbstractProtocol::ProtocolIdLlc,
        ProtocolIdEth = AbstractProtocol::ProtocolIdEth,
        ProtocolIdIp = AbstractProtocol::ProtocolIdIp
    };

    UserProtocol(AbstractProtocol *parent);

    bool isProtocolFrameFixed() const;
    const QByteArray& protocolFrameFixedValue() const;
    void setProtocolFrameFixedValue(const QByteArray& value);
    void setProtocolFrameFixedValue(const QList<int>& value);

public slots:
    void reset();

    QString name() const;
    void setName(QString &name);
    QString shortName() const;
    void setShortName(QString &shortName);

    bool isProtocolFrameValueVariable() const;
    void setProtocolFrameValueVariable(bool variable);
    bool isProtocolFrameSizeVariable() const;
    void setProtocolFrameSizeVariable(bool variable);

    quint32 payloadProtocolId(UserProtocol::ProtocolIdType type) const;
    int protocolFrameOffset(int streamIndex = 0) const;
    int protocolFramePayloadSize(int streamIndex = 0) const;

    bool isProtocolFramePayloadValueVariable() const;
    bool isProtocolFramePayloadSizeVariable() const;

    quint32 protocolFrameHeaderCksum(int streamIndex = 0,
        AbstractProtocol::CksumType cksumType = AbstractProtocol::CksumIp) const;
    quint32 protocolFramePayloadCksum(int streamIndex = 0,
        AbstractProtocol::CksumType cksumType = AbstractProtocol::CksumIp) const;

private:
    AbstractProtocol *_parent;

    QString _name;
    QString _shortName;
    QMap<int, quint32> _protocolId;
    bool _protocolFrameValueVariable;
    bool _protocolFrameSizeVariable;
    QByteArray _protocolFrameFixedValue;

};

class UserScriptConfigForm : public QWidget, public Ui::UserScript
{
    Q_OBJECT

public:
    UserScriptConfigForm(UserScriptProtocol *protocol, QWidget *parent = 0);

private:
    UserScriptProtocol        *_protocol;

private slots:
    void on_programEdit_textChanged();
    void on_compileButton_clicked(bool checked = false);
};


class UserScriptProtocol : public AbstractProtocol
{
    friend class UserScriptConfigForm;

public:
    UserScriptProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
    virtual ~UserScriptProtocol();

    static AbstractProtocol* createInstance(StreamBase *stream,
        AbstractProtocol *parent = 0);
    virtual quint32 protocolNumber() const;

    virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
    virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

    virtual quint32 protocolId(ProtocolIdType type) const;

    virtual QString name() const;
    virtual QString shortName() const;

    virtual int fieldCount() const;

    virtual QVariant fieldData(int index, FieldAttrib attrib,
            int streamIndex = 0) const;
    virtual bool setFieldData(int index, const QVariant &value, 
            FieldAttrib attrib = FieldValue);

    virtual bool isProtocolFrameValueVariable() const;
    virtual bool isProtocolFrameSizeVariable() const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

private:
    bool evaluateUserScript() const;
    int userScriptErrorLineNumber() const;
    QString userScriptErrorText() const;

    int userScriptLineCount() const;


    OstProto::UserScript    data;
    UserScriptConfigForm    *configForm;
    enum userScriptfield
    {
        // Frame Fields
        userScript_program = 0,

        userScript_fieldCount
    };

    mutable QScriptEngine   _scriptEngine;
    mutable UserProtocol    _userProtocol;
    mutable QScriptValue    _userProtocolScriptValue;

    mutable bool            _isUpdated;
    mutable int             _errorLineNumber;
    mutable QString         _errorText;
};

#endif

/* vim: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab: */
