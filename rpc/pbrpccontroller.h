#ifndef _PB_RPC_CONTROLLER_H
#define _PB_RPC_CONTROLLER_H

#include <google/protobuf/service.h>

class QIODevice;

class PbRpcController : public ::google::protobuf::RpcController
{
    bool        failed;
    QIODevice    *blob;
    std::string    errStr;

public:
    PbRpcController() { Reset(); }

    // Client Side Methods
    void Reset() { failed=false; blob = NULL; }
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
};

#endif
