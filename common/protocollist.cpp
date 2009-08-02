#include "protocollist.h"
#include "abstractprotocol.h"

void ProtocolList::destroy() 
{
	while (!isEmpty())
		delete takeFirst(); 
}
