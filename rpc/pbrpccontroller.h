/*
Copyright (C) 2010, 2014 Srivats P.

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

#ifndef _PB_RPC_CONTROLLER_H
#define _PB_RPC_CONTROLLER_H

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

class QIODevice;

/*!
PbRpcController takes ownership of the 'request' and 'response' messages and
will delete them when it itself is destroyed
*/
class PbRpcController : public ::google::protobuf::RpcController
{
public:
    PbRpcController(::google::protobuf::Message *request, 
            ::google::protobuf::Message *response) { 
        request_ = request;
        response_ = response;
        Reset(); 
    }
    ~PbRpcController() { delete request_; delete response_; }

    ::google::protobuf::Message* request() { return request_; }
    ::google::protobuf::Message* response() { return response_; }

    // Client Side Methods
    void Reset() { failed = false; blob = NULL; }
    bool Failed() const { return failed; }
    void StartCancel() { /*! \todo (MED) */}
    std::string ErrorText() const { return errStr; }

    // Server Side Methods
    void SetFailed(const std::string &reason) 
        { failed = true; errStr = reason; }
    bool IsCanceled() const { return false; };
    void NotifyOnCancel(::google::protobuf::Closure* /* callback */) {
        /*! \todo (MED) */ 
    }

    // srivatsp added
    QIODevice* binaryBlob() { return blob; };
    void setBinaryBlob(QIODevice *binaryBlob) { blob = binaryBlob; };

private:
    bool failed;
    QIODevice *blob;
    std::string errStr;
    ::google::protobuf::Message *request_;
    ::google::protobuf::Message *response_;

};

#endif
