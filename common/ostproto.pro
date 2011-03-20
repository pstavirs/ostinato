TEMPLATE = lib
CONFIG += qt staticlib
QT += network script xml
INCLUDEPATH += "../extra/qhexedit2/src"
LIBS += \
    -lprotobuf
FORMS += \
    pcapfileimport.ui \
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
    hexdump.ui \
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
    hexdump.proto \
    sample.proto 
HEADERS += \
    ostprotolib.h \
    abstractprotocol.h    \
    comboprotocol.h    \
    abstractfileformat.h \
    fileformat.h \
    pcapfileformat.h \
    pdmlfileformat.h \
    pdml_p.h \
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
    ipv4addressdelegate.h \
    ipv6addressdelegate.h \
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
    hexdump.h \
    sample.h
SOURCES += \
    ostprotolib.cpp \
    abstractprotocol.cpp \
    crc32c.cpp \
    abstractfileformat.cpp \
    fileformat.cpp \
    pcapfileformat.cpp \
    pdmlfileformat.cpp \
    pdml_p.cpp \
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
    hexdump.cpp \
    sample.cpp

QMAKE_DISTCLEAN += object_script.*

include(../protobuf.pri)
