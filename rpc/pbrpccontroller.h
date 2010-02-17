#ifndef _PB_RPC_CONTROLLER_H
#define _PB_RPC_CONTROLLER_H

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
