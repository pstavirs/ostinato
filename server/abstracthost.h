#ifndef _ABSTRACT_HOST
#define _ABSTRACT_HOST

class AbstractHost
{
	public:
		virtual void Log(const char* str) = 0;
#if 0 // PB
		virtual int SendMsg(const void* msg, int size) = 0;
#endif
};

#endif

