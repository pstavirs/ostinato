TEMPLATE = lib
CONFIG += qt staticlib
QT += widgets network xml script
INCLUDEPATH += "../extra/qhexedit2/src"

FORMS += \
    mac.ui \
    vlan.ui \
    eth2.ui \
    dot3.ui \
    llc.ui \
    snap.ui \
    stp.ui \
    arp.ui \
    ip4.ui \
    ip6.ui \
    gmp.ui \
    gre.ui \
    icmp.ui \
    tcp.ui \
    udp.ui \
    textproto.ui \
    tosdscp.ui \
    hexdump.ui \
    payload.ui \
    sample.ui \
    sign.ui \
    userscript.ui

HEADERS = \
    ipv4addressdelegate.h \
    ipv6addressdelegate.h \
    spinboxdelegate.h \
    tosdscp.h

HEADERS += \
    abstractprotocolconfig.h \
    comboprotocolconfig.h \
    protocolwidgetfactory.h \
    macconfig.h \ 
    vlanconfig.h \
    svlanconfig.h \
    vlanstackconfig.h \
    eth2config.h \
    dot3config.h \
    llcconfig.h \
    dot2llcconfig.h \
    snapconfig.h \
    dot2snapconfig.h \
    stpconfig.h \
    arpconfig.h \
    ip4config.h \
    ip6config.h \
    ip4over4config.h \
    gmpconfig.h \
    greconfig.h \
    icmpconfig.h \
    igmpconfig.h \
    mldconfig.h \
    tcpconfig.h \
    udpconfig.h \
    textprotoconfig.h \
    hexdumpconfig.h \
    payloadconfig.h \
    sampleconfig.h \
    signconfig.h \
    userscriptconfig.h

SOURCES += \
    tosdscp.cpp

SOURCES += \
    protocolwidgetfactory.cpp \
    macconfig.cpp \
    vlanconfig.cpp \
    eth2config.cpp \
    dot3config.cpp \
    llcconfig.cpp \
    snapconfig.cpp \
    stpconfig.cpp \
    arpconfig.cpp \
    ip4config.cpp \
    ip6config.cpp \
    gmpconfig.cpp \
    greconfig.cpp \
    icmpconfig.cpp \
    igmpconfig.cpp \
    mldconfig.cpp \
    tcpconfig.cpp \
    udpconfig.cpp \
    textprotoconfig.cpp \
    hexdumpconfig.cpp \
    payloadconfig.cpp \
    sampleconfig.cpp \
    signconfig.cpp \
    userscriptconfig.cpp

QMAKE_DISTCLEAN += object_script.*

include(../options.pri)
