TEMPLATE = lib
CONFIG += qt staticlib
QT -= gui
QT += network script
LIBS += \
    -lprotobuf

PROTOS = \
    protocol.proto \
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
    hexdump.proto \
    sample.proto 

HEADERS = \
    abstractprotocol.h    \
    comboprotocol.h    \
    protocolmanager.h \
    protocollist.h \
    protocollistiterator.h \
    streambase.h \

HEADERS += \
    mac.h \
    vlan.h \
    svlan.h \
    vlanstack.h \
    eth2.h \
    dot3.h \
    llc.h \
    dot2llc.h \
    snap.h \
    dot2snap.h \
    arp.h \
    ip4.h \
    ip6.h \
    ip4over4.h \
    ip4over6.h \
    ip6over4.h \
    ip6over6.h \
    gmp.h \
    icmp.h \
    igmp.h \
    mld.h \
    tcp.h \
    udp.h \
    textproto.h \
    hexdump.h \
    payload.h \
    sample.h \
    userscript.h 

SOURCES = \
    abstractprotocol.cpp \
    crc32c.cpp \
    protocolmanager.cpp \
    protocollist.cpp \
    protocollistiterator.cpp \
    streambase.cpp \

SOURCES += \
    mac.cpp \
    vlan.cpp \
    svlan.cpp \
    eth2.cpp \
    dot3.cpp \
    llc.cpp \
    snap.cpp \
    arp.cpp \
    ip4.cpp \
    ip6.cpp \
    gmp.cpp \
    icmp.cpp \
    igmp.cpp \
    mld.cpp \
    tcp.cpp \
    udp.cpp \
    textproto.cpp \
    hexdump.cpp \
    payload.cpp \
    sample.cpp \
    userscript.cpp

QMAKE_DISTCLEAN += object_script.*

#binding.depends = compiler_protobuf_py_make_all
#QMAKE_EXTRA_TARGETS += binding

include(../protobuf.pri)

