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
    Q_OBJECT;
    Q_ENUMS(ProtocolIdType);
    Q_ENUMS(CksumType);

    Q_PROPERTY(QString name READ name WRITE setName);
    Q_PROPERTY(bool protocolFrameValueVariable
            READ isProtocolFrameValueVariable
            WRITE setProtocolFrameValueVariable);
    Q_PROPERTY(bool protocolFrameSizeVariable
            READ isProtocolFrameSizeVariable
            WRITE setProtocolFrameSizeVariable);
    Q_PROPERTY(int protocolFrameVariableCount
            READ protocolFrameVariableCount
            WRITE setProtocolFrameVariableCount);
    
public:
    enum ProtocolIdType
    {
        ProtocolIdLlc = AbstractProtocol::ProtocolIdLlc,
        ProtocolIdEth = AbstractProtocol::ProtocolIdEth,
        ProtocolIdIp = AbstractProtocol::ProtocolIdIp,
        ProtocolIdTcpUdp = AbstractProtocol::ProtocolIdTcpUdp
    };

    enum CksumType
    {
        CksumIp = AbstractProtocol::CksumIp,
        CksumIpPseudo = AbstractProtocol::CksumIpPseudo,
        CksumTcpUdp = AbstractProtocol::CksumTcpUdp
    };

    UserProtocol(AbstractProtocol *parent);

public slots:
    void reset();

    QString name() const;
    void setName(QString &name);

    bool isProtocolFrameValueVariable() const;
    void setProtocolFrameValueVariable(bool variable);
    bool isProtocolFrameSizeVariable() const;
    void setProtocolFrameSizeVariable(bool variable);
    int protocolFrameVariableCount() const;
    void setProtocolFrameVariableCount(int count);

    quint32 payloadProtocolId(UserProtocol::ProtocolIdType type) const;
    int protocolFrameOffset(int streamIndex = 0) const;
    int protocolFramePayloadSize(int streamIndex = 0) const;

    bool isProtocolFramePayloadValueVariable() const;
    bool isProtocolFramePayloadSizeVariable() const;
    int protocolFramePayloadVariableCount() const;

    quint32 protocolFrameHeaderCksum(int streamIndex = 0,
        AbstractProtocol::CksumType cksumType = AbstractProtocol::CksumIp) const;
    quint32 protocolFramePayloadCksum(int streamIndex = 0,
        AbstractProtocol::CksumType cksumType = AbstractProtocol::CksumIp) const;

private:
    AbstractProtocol *parent_;

    QString name_;
    bool protocolFrameValueVariable_;
    bool protocolFrameSizeVariable_;
    int protocolFrameVariableCount_;
};



class UserScriptConfigForm : public QWidget, public Ui::UserScript
{
    Q_OBJECT

public:
    UserScriptConfigForm(UserScriptProtocol *protocol, QWidget *parent = 0);

private:
    void updateStatus();
    UserScriptProtocol        *protocol_;

private slots:
    void on_programEdit_textChanged();
    void on_compileButton_clicked(bool checked = false);
};



class UserScriptProtocol : public AbstractProtocol
{

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

    virtual int protocolFrameSize(int streamIndex = 0) const;

    virtual bool isProtocolFrameValueVariable() const;
    virtual bool isProtocolFrameSizeVariable() const;
    virtual int protocolFrameVariableCount() const;

    virtual quint32 protocolFrameCksum(int streamIndex = 0,
            CksumType cksumType = CksumIp) const;

    virtual QWidget* configWidget();
    virtual void loadConfigWidget();
    virtual void storeConfigWidget();

    void evaluateUserScript() const;
    bool isScriptValid() const;
    int userScriptErrorLineNumber() const;
    QString userScriptErrorText() const;

private:
    int userScriptLineCount() const;

    enum userScriptfield
    {
        // Frame Fields
        userScript_program = 0,

        userScript_fieldCount
    };
    OstProto::UserScript    data;
    UserScriptConfigForm    *configForm;

    mutable QScriptEngine   engine_;
    mutable UserProtocol    userProtocol_;
    mutable QScriptValue    userProtocolScriptValue_;

    mutable bool            isScriptValid_;
    mutable int             errorLineNumber_;
    mutable QString         errorText_;
};

#endif

/* vim: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab: */
