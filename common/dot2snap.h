#ifndef _DOT2_SNAP_H
#define _DOT2_SNAP_H

#include "comboprotocol.h"
#include "dot2llc.h"
#include "snap.h"

typedef ComboProtocol<OstProto::Protocol::kDot2SnapFieldNumber, 
    Dot2LlcProtocol, SnapProtocol> Dot2SnapProtocol;

#endif
