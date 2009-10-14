#ifndef _VLAN_STACK_H
#define _VLAN_STACK_H

#include "comboprotocol.h"
#include "vlan.h"

typedef ComboProtocol<OstProto::Protocol::kVlanStackFieldNumber, 
	VlanProtocol, VlanProtocol> VlanStackProtocol;

#endif
