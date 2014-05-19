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

#include <qendian.h>

PbRpcChannel::PbRpcChannel(QHostAddress ip, quint16 port)
{
    isPending = false;
    pendingMethodId = -1;    // don't care as long as isPending is false

    controller = NULL;
    done = NULL;
    response = NULL;

    mServerAddress = ip;
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

    mpSocket->connectToHost(mServerAddress, mServerPort);
}

void PbRpcChannel::establish(QHostAddress ip, quint16 port)
{
    mServerAddress = ip;
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
    char    msgBuf[PB_HDR_SIZE];
    char* const msg = &msgBuf[0];
    int     len;
    bool    ret;
  
    if (isPending)
    {
        RpcCall call;
        qDebug("RpcChannel: queueing rpc since method %d is pending;<----\n "
                "queued method = %d\n"
                "queued message = \n%s\n---->", 
                pendingMethodId, method->index(), req->DebugString().c_str());

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
        qDebug("req = \n%s", req->DebugString().c_str());
        qDebug("error = \n%s\n--->", req->InitializationErrorString().c_str());

        controller->SetFailed("Required fields missing");
        done->Run();
        return;
    }

    pendingMethodId = method->index();
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
        qDebug("method = %d\n req = \n%s\n---->", 
                method->index(), req->DebugString().c_str());
    }

    mpSocket->write(msg, PB_HDR_SIZE);
    ret = req->SerializeToZeroCopyStream(outStream);
    Q_ASSERT(ret == true);
    outStream->Flush();
}

void PbRpcChannel::on_mpSocket_readyRead()
{
    uchar   msg[PB_HDR_SIZE];
    uchar   *p = (uchar*) &msg;
    int        msgLen;
    static bool parsing = false;
    static quint16    type, method;
    static quint32    len;

    //qDebug("%s: bytesAvail = %d", __FUNCTION__, mpSocket->bytesAvailable());

    if (!parsing)
    {
        // Do we have an entire header? If not, we'll wait ...
        if (mpSocket->bytesAvailable() < PB_HDR_SIZE)
        {
            qDebug("client: not enough data available for a complete header");
            return;
        }

        msgLen = mpSocket->read((char*)msg, PB_HDR_SIZE);

        Q_ASSERT(msgLen == PB_HDR_SIZE);

        type = qFromBigEndian<quint16>(p+0);
        method = qFromBigEndian<quint16>(p+2);
        len = qFromBigEndian<quint32>(p+4);

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

            blob = static_cast<PbRpcController*>(controller)->binaryBlob();
            Q_ASSERT(blob != NULL);

            while ((cumLen < len) && mpSocket->bytesAvailable())
            {
                int l;

                l = mpSocket->read((char*)msg, sizeof(msg));
                blob->write((char*)msg, l);
                cumLen += l;
            }

            qDebug("%s: bin blob rcvd %d/%d", __PRETTY_FUNCTION__, cumLen, len);

            if (cumLen < len)
                return;

            cumLen = 0;

            if (!isPending)
            {
                qDebug("not waiting for response");
                goto _error_exit2;
            }

            if (pendingMethodId != method)
            {
                qDebug("invalid method id %d (expected = %d)", method, 
                    pendingMethodId);
                goto _error_exit2;
            }

            break;
        }

        case PB_MSG_TYPE_RESPONSE:
            //qDebug("client(%s) rcvd %d bytes", __FUNCTION__, msgLen);
            //BUFDUMP(msg, msgLen);

            if (!isPending)
            {
                qDebug("not waiting for response");
                goto _error_exit;
            }

            if (pendingMethodId != method)
            {
                qDebug("invalid method id %d (expected = %d)", method, 
                    pendingMethodId);
                goto _error_exit;
            }

            if (len)
                response->ParseFromBoundedZeroCopyStream(inStream, len);

            // Avoid printing stats
            if (method != 13)
            {
                qDebug("client(%s): Received Msg <---- ", __FUNCTION__);
                qDebug("method = %d\nresp = \n%s\n---->",
                        method, response->DebugString().c_str());
            }

            if (!response->IsInitialized())
            {
                qWarning("RpcChannel: missing required fields in response <----");
                qDebug("resp = \n%s", response->DebugString().c_str());
                qDebug("error = \n%s\n--->", 
                        response->InitializationErrorString().c_str());

                controller->SetFailed("Required fields missing");
            }
            break;

        default:
            qFatal("%s: unexpected type %d", __PRETTY_FUNCTION__, type);
            goto _error_exit;
                
    }

    done->Run();

    pendingMethodId = -1;
    controller = NULL;
    response = NULL;
    isPending = false;
    parsing = false;

    if (pendingCallList.size())
    {
        RpcCall call = pendingCallList.takeFirst();
        qDebug("RpcChannel: executing queued method <----\n"
               "method = %d\n"
               "req = \n%s\n---->", 
                call.method->index(), call.request->DebugString().c_str());
        CallMethod(call.method, call.controller, call.request, call.response,
                call.done);
    }

    return;

_error_exit:
    inStream->Skip(len);
_error_exit2:
    parsing = false;
    qDebug("client(%s) discarding received msg <----", __FUNCTION__);
    qDebug("method = %d\nreq = \n%s\n---->",
            method, response->DebugString().c_str());
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

