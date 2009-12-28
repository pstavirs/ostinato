#ifndef _DOT2_LLC_H
#define _DOT2_LLC_H

#include "comboprotocol.h"
#include "dot3.h"
#include "llc.h"

typedef ComboProtocol<OstProto::Protocol::kDot2LlcFieldNumber, 
    Dot3Protocol, LlcProtocol> Dot2LlcProtocol;

#endif
