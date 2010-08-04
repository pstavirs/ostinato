TEMPLATE = lib
CONFIG += qt staticlib
QT += network script
LIBS += \
    -lprotobuf
FORMS += \
    mac.ui \
    payload.ui \
    eth2.ui \
    dot3.ui \
    llc.ui \
    snap.ui \
    vlan.ui \
    arp.ui \
    ip4.ui \
    ip6.ui \
    icmp.ui \
    gmp.ui \
    tcp.ui \
    udp.ui \
    textproto.ui \
    userscript.ui \
    sample.ui 
PROTOS += \
    protocol.proto \
    fileformat.proto \
    mac.proto \
    payload.proto \
    eth2.proto \
    dot3.proto \
    llc.proto \
    snap.proto \
    dot2llc.proto \
    dot2snap.proto \
    vlan.proto \
    svlan.proto \
    vlanstack.proto \
    arp.proto \
    ip4.proto \
    ip6.proto \
    ip6over4.proto \
    ip4over6.proto \
    ip4over4.proto \
    ip6over6.proto \
    icmp.proto \
    gmp.proto \
    igmp.proto \
    mld.proto \
    tcp.proto \
    udp.proto \
    textproto.proto \
    userscript.proto \
    sample.proto 
HEADERS += \
    abstractprotocol.h    \
    comboprotocol.h    \
    fileformat.h \
    protocolmanager.h \
    protocollist.h \
    protocollistiterator.h \
    streambase.h \
    mac.h \
    payload.h \
    eth2.h \
    dot3.h \
    llc.h \
    snap.h \
    dot2llc.h \
    dot2snap.h \
    vlan.h \
    svlan.h \
    vlanstack.h \
    arp.h \
    ip4.h \
    ip6.h \
    ip6over4.h \
    ip4over6.h \
    ip4over4.h \
    ip6over6.h \
    icmp.h \
    gmp.h \
    igmp.h \
    mld.h \
    tcp.h \
    udp.h \
    textproto.h \
    userscript.h \
    sample.h
SOURCES += \
    abstractprotocol.cpp \
    crc32c.cpp \
    fileformat.cpp \
    protocolmanager.cpp \
    protocollist.cpp \
    protocollistiterator.cpp \
    streambase.cpp \
    mac.cpp \
    payload.cpp \
    eth2.cpp \
    dot3.cpp \
    llc.cpp \
    snap.cpp \
    vlan.cpp \
    svlan.cpp \
    arp.cpp \
    ip4.cpp \
    ip6.cpp \
    icmp.cpp \
    gmp.cpp \
    igmp.cpp \
    mld.cpp \
    tcp.cpp \
    udp.cpp \
    textproto.cpp \
    userscript.cpp \
    sample.cpp

QMAKE_DISTCLEAN += object_script.*

include(../protobuf.pri)
