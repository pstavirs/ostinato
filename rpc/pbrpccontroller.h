#ifndef _PB_RPC_CONTROLLER_H
#define _PB_RPC_CONTROLLER_H

#include <google/protobuf/service.h>

class PbRpcController : public ::google::protobuf::RpcController
{
	bool		failed;
	std::string	errStr;

public:
	PbRpcController() { failed = false; }

	// Client Side Methods
	void Reset() { failed=false;}
	bool Failed() const { return failed; }
	void StartCancel() { /* TODO */}
	std::string ErrorText() const { return errStr; }

	// Server Side Methods
	void SetFailed(const std::string &reason) 
		{ failed = true; errStr = reason; }
	bool IsCanceled() const { return false; };
	void NotifyOnCancel(::google::protobuf::Closure *callback) { /*TODO*/ }
};

#endif
