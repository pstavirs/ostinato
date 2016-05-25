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

#include "pbrpcchannel.h"
#include "pbqtio.h"

#include <QtGlobal>
#include <qendian.h>

static uchar msgBuf[4096];

PbRpcChannel::PbRpcChannel(QString serverName, quint16 port,
                           const ::google::protobuf::Message &notifProto)
    : notifPrototype(notifProto)
{
    isPending = false;
    pendingMethodId = -1;    // don't care as long as isPending is false

    method = NULL;
    controller = NULL;
    done = NULL;
    response = NULL;

    mServerHost = serverName;
    mServerPort = port;
    mpSocket = new QTcpSocket(this);

    inStream = new google::protobuf::io::CopyingInputStreamAdaptor(
            new PbQtInputStream(mpSocket));
    inStream->SetOwnsCopyingStream(true);
    outStream = new google::protobuf::io::CopyingOutputStreamAdaptor(
            new PbQtOutputStream(mpSocket));
    outStream->SetOwnsCopyingStream(true);

    // FIXME: Not quite sure why this ain't working!
    // QMetaObject::connectSlotsByName(this);

    connect(mpSocket, SIGNAL(connected()),
        this, SLOT(on_mpSocket_connected()));
    connect(mpSocket, SIGNAL(disconnected()),
        this, SLOT(on_mpSocket_disconnected()));
    connect(mpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
        this, SLOT(on_mpSocket_stateChanged(QAbstractSocket::SocketState)));
    connect(mpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(on_mpSocket_error(QAbstractSocket::SocketError)));

    connect(mpSocket, SIGNAL(readyRead()),
        this, SLOT(on_mpSocket_readyRead()));

}

PbRpcChannel::~PbRpcChannel()
{
    delete inStream;
    delete outStream;
    delete mpSocket;
}

void PbRpcChannel::establish()
{
    qDebug("In %s", __FUNCTION__);

    mpSocket->connectToHost(mServerHost, mServerPort);
}

void PbRpcChannel::establish(QString serverName, quint16 port)
{
    mServerHost = serverName;
    mServerPort = port;
    establish();
}

void PbRpcChannel::tearDown()
{
    qDebug("In %s", __FUNCTION__);

    mpSocket->disconnectFromHost();
}

void PbRpcChannel::CallMethod(
    const ::google::protobuf::MethodDescriptor *method,
    ::google::protobuf::RpcController *controller,
    const ::google::protobuf::Message *req,
    ::google::protobuf::Message *response,
    ::google::protobuf::Closure* done)
{
    char* msg = (char*) &msgBuf[0];
    int     len;
    bool    ret;
  
    if (isPending)
    {
        RpcCall call;
        qDebug("RpcChannel: queueing rpc since method %d is pending;<----\n "
                "queued method = %d:%s\n"
                "queued message = \n%s\n---->", 
                pendingMethodId, method->index(), method->name().c_str(),
                req->DebugString().c_str());

        call.method = method;
        call.controller = controller;
        call.request = req;
        call.response = response;
        call.done = done;

        pendingCallList.append(call);
	qDebug("pendingCallList size = %d", pendingCallList.size());

        Q_ASSERT(pendingCallList.size() < 100);

        return;
    }

    if (!req->IsInitialized())
    {
        qWarning("RpcChannel: missing required fields in request <----");
        qDebug("req = %s\n%s", method->input_type()->name().c_str(),
                req->DebugString().c_str());
        qDebug("error = \n%s\n--->", req->InitializationErrorString().c_str());

        controller->SetFailed("Required fields missing");
        done->Run();
        return;
    }

    pendingMethodId = method->index();
    this->method=method;
    this->controller=controller;
    this->done=done;
    this->response=response;
    isPending = true;

    len = req->ByteSize();
    *((quint16*)(msg+0)) = qToBigEndian(quint16(PB_MSG_TYPE_REQUEST)); // type
    *((quint16*)(msg+2)) = qToBigEndian(quint16(method->index())); // method id
    *((quint32*)(msg+4)) = qToBigEndian(quint32(len)); // len

    // Avoid printing stats since it happens every couple of seconds
    if (pendingMethodId != 13)
    {
        qDebug("client(%s) sending %d bytes <----", __FUNCTION__, 
                PB_HDR_SIZE + len);
        BUFDUMP(msg, PB_HDR_SIZE);
        qDebug("method = %d:%s\n req = %s\n%s\n---->",
                method->index(), method->name().c_str(),
                method->input_type()->name().c_str(),
                req->DebugString().c_str());
    }

    mpSocket->write(msg, PB_HDR_SIZE);
    ret = req->SerializeToZeroCopyStream(outStream);
    Q_ASSERT(ret == true);
    Q_UNUSED(ret);
    outStream->Flush();
}

void PbRpcChannel::on_mpSocket_readyRead()
{
    const uchar      *msg;
    int               msgLen;
    static bool parsing = false;
    static quint16    type, method;
    static quint32    len;

_top:
    //qDebug("%s(entry): bytesAvail = %d", __FUNCTION__, mpSocket->bytesAvailable());

    if (!parsing)
    {
        // Do we have an entire header? If not, we'll wait ...
        if (inStream->Next((const void**)&msg, &msgLen) == false) {
            qDebug("No more data or stream error");
            goto _exit;
        }

        if (msgLen < PB_HDR_SIZE) {
            qDebug("read less than PB_HDR_SIZE bytes; putting back");
            inStream->BackUp(msgLen);
            goto _exit;
        }

        type = qFromBigEndian<quint16>(msg+0);
        method = qFromBigEndian<quint16>(msg+2);
        len = qFromBigEndian<quint32>(msg+4);

        if (msgLen > PB_HDR_SIZE)
            inStream->BackUp(msgLen - PB_HDR_SIZE);

        //BUFDUMP(msg, PB_HDR_SIZE);
        //qDebug("type = %hu, method = %hu, len = %u", type, method, len);

        parsing = true;
    }

    switch (type)
    {
        case PB_MSG_TYPE_BINBLOB:
        {
            static quint32 cumLen = 0;
            QIODevice *blob;
            int l = 0;

            blob = static_cast<PbRpcController*>(controller)->binaryBlob();
            Q_ASSERT(blob != NULL);

            msgLen = 0;
            while (cumLen < len)
            {
                if (inStream->Next((const void**)&msg, &msgLen) == false) {
                    //qDebug("No more data or stream error");
                    goto _exit;
                }

                l = qMin(msgLen, int(len - cumLen));
                blob->write((char*)msg, l);
                cumLen += l;
                //qDebug("%s: bin blob rcvd %d/%d/%d", __PRETTY_FUNCTION__, l, cumLen, len);
            }

            if (l < msgLen) {
                qDebug("read extra bytes after blob; putting back");
                inStream->BackUp(msgLen - l);
            }

            qDebug("%s: bin blob rcvd %d/%d", __PRETTY_FUNCTION__, cumLen, len);

            if (cumLen < len)
                goto _exit;

            cumLen = 0;

            if (!isPending)
            {
                qWarning("not waiting for response");
                goto _error_exit2;
            }

            if (pendingMethodId != method)
            {
                qWarning("invalid method id %d (expected = %d)", method, 
                    pendingMethodId);
                goto _error_exit2;
            }

            break;
        }

        case PB_MSG_TYPE_RESPONSE:
        {
            static quint32 cumLen = 0;
            static QByteArray buffer;
            int l = 0;

            if (!isPending)
            {
                qWarning("not waiting for response");
                goto _error_exit;
            }

            if (pendingMethodId != method)
            {
                qWarning("invalid method id %d (expected = %d)", method, 
                    pendingMethodId);
                goto _error_exit;
            }

            msgLen = 0;
            while (cumLen < len)
            {
                if (inStream->Next((const void**)&msg, &msgLen) == false) {
                    //qDebug("No more data or stream error");
                    goto _exit;
                }

                l = qMin(msgLen, int(len - cumLen));
                buffer.append(QByteArray((char*)msg, l));
                cumLen += l;
                //qDebug("%s: buffer rcvd %d/%d/%d", __PRETTY_FUNCTION__, l, cumLen, len);
            }

            if (l < msgLen) {
                qDebug("read extra bytes after response; putting back");
                inStream->BackUp(msgLen - l);
            }

#if 0 // not needed?
            if (cumLen < len)
                goto _exit;
#endif

            if (len)
                response->ParseFromArray((const void*)buffer, len);

            cumLen = 0;
            buffer.resize(0);

            // Avoid printing stats
            if (method != 13)
            {
                qDebug("client(%s): Received Msg <---- ", __FUNCTION__);
                qDebug("method = %d:%s\nresp = %s\n%s\n---->",
                        method, this->method->name().c_str(),
                        this->method->output_type()->name().c_str(),
                        response->DebugString().c_str());
            }

            if (!response->IsInitialized())
            {
                qWarning("RpcChannel: missing required fields in response <----");
                qDebug("resp = %s\n%s",
                        this->method->output_type()->name().c_str(),
                        response->DebugString().c_str());
                qDebug("error = \n%s\n--->", 
                        response->InitializationErrorString().c_str());

                controller->SetFailed("Required fields missing");
            }
            break;
        }
        case PB_MSG_TYPE_ERROR:
        {
            static quint32 cumLen = 0;
            static QByteArray error;
            int l = 0;

            msgLen = 0;
            while (cumLen < len)
            {
                if (inStream->Next((const void**)&msg, &msgLen) == false) {
                    //qDebug("No more data or stream error");
                    goto _exit;
                }

                l = qMin(msgLen, int(len - cumLen));
                error.append(QByteArray((char*)msg, l));
                cumLen += l;
                //qDebug("%s: error rcvd %d/%d/%d", __PRETTY_FUNCTION__, l, cumLen, len);
            }

            if (l < msgLen) {
                qDebug("read extra bytes after error; putting back");
                inStream->BackUp(msgLen - l);
            }

            qDebug("%s: error rcvd %d/%d", __PRETTY_FUNCTION__, cumLen, len);

            if (cumLen < len)
                goto _exit;

            static_cast<PbRpcController*>(controller)->SetFailed(
                    QString::fromUtf8(error, len));

            cumLen = 0;
            error.resize(0);

            if (!isPending)
            {
                qWarning("not waiting for response");
                goto _error_exit2;
            }

            if (pendingMethodId != method)
            {
                qWarning("invalid method id %d (expected = %d)", method, 
                    pendingMethodId);
                goto _error_exit2;
            }

            break;
        }

        case PB_MSG_TYPE_NOTIFY: 
        {
            notif = notifPrototype.New();
            if (!notif)
            {
                qWarning("failed to alloc notify");
                goto _error_exit;
            }

            if (len)
                notif->ParseFromBoundedZeroCopyStream(inStream, len);

            qDebug("client(%s): Received Notif Msg <---- ", __FUNCTION__);
            qDebug("type = %d\nnotif = \n%s\n---->",
                    method, notif->DebugString().c_str());

            if (!notif->IsInitialized())
            {
                qWarning("RpcChannel: missing required fields in notify <----");
                qDebug("notify = \n%s", notif->DebugString().c_str());
                qDebug("error = \n%s\n--->", 
                        notif->InitializationErrorString().c_str());
            }
            else 
                emit notification(method, notif);

            delete notif;
            notif = NULL;

            parsing = false;
            goto _exit;

            break;
        }

        default:
            qFatal("%s: unexpected type %d", __PRETTY_FUNCTION__, type);
            goto _error_exit;
                
    }

    done->Run();

    pendingMethodId = -1;
    this->method = NULL;
    controller = NULL;
    response = NULL;
    isPending = false;
    parsing = false;

    if (pendingCallList.size())
    {
        RpcCall call = pendingCallList.takeFirst();
        qDebug("RpcChannel: executing queued method <----\n"
               "method = %d:%s\n"
               "req = %s\n%s\n---->",
                call.method->index(), call.method->name().c_str(),
                call.method->input_type()->name().c_str(),
                call.request->DebugString().c_str());
        CallMethod(call.method, call.controller, call.request, call.response,
                call.done);
    }

    goto _exit;

_error_exit:
    inStream->Skip(len);
_error_exit2:
    parsing = false;
    qDebug("client(%s) discarding received msg <----", __FUNCTION__);
    qDebug("method = %d\n---->", method);
_exit:
    // If we have some data still available continue reading/parsing
    if (inStream->Next((const void**)&msg, &msgLen)) {
        if (msgLen >= PB_HDR_SIZE) {
            inStream->BackUp(msgLen);
            qDebug("===>> MORE DATA PENDING (%d bytes)... CONTINUE", msgLen);
            goto _top;
        }
    }
    if (mpSocket->bytesAvailable())
        qDebug("%s (exit): bytesAvail = %lld", __FUNCTION__, mpSocket->bytesAvailable());
    return;
}

void PbRpcChannel::on_mpSocket_stateChanged(
    QAbstractSocket::SocketState socketState)
{
    qDebug("In %s", __FUNCTION__);
    emit stateChanged(socketState);
}

void PbRpcChannel::on_mpSocket_connected()
{
    qDebug("In %s", __FUNCTION__);
    emit connected(); 
}

void PbRpcChannel::on_mpSocket_disconnected()
{
    qDebug("In %s", __FUNCTION__);

    pendingMethodId = -1;
    method = NULL;
    controller = NULL;
    response = NULL;
    isPending = false;
    // \todo convert parsing from static to data member
    //parsing = false 
    pendingCallList.clear();

    emit disconnected();
}

void PbRpcChannel::on_mpSocket_error(QAbstractSocket::SocketError socketError)
{
    qDebug("In %s", __FUNCTION__);
    emit error(socketError);
}

