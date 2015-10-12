TEMPLATE = lib
CONFIG += qt staticlib
QT += network xml script
INCLUDEPATH += "../extra/qhexedit2/src"
LIBS += \
    -lprotobuf

FORMS = \
    pcapfileimport.ui \

FORMS += \
    mac.ui \
    vlan.ui \
    eth2.ui \
    dot3.ui \
    llc.ui \
    snap.ui \
    lacp.ui \
    arp.ui \
    ip4.ui \
    ip6.ui \
    gmp.ui \
    icmp.ui \
    tcp.ui \
    udp.ui \
    textproto.ui \
    hexdump.ui \
    payload.ui \
    sample.ui \
    userscript.ui

PROTOS = \
    fileformat.proto 

# TODO: Move fileformat related stuff into a different library - why?
HEADERS = \
    ostprotolib.h \
    abstractfileformat.h \
    fileformat.h \
    ipv4addressdelegate.h \
    ipv6addressdelegate.h \
    pcapfileformat.h \
    pdmlfileformat.h \
    pdmlprotocol.h \
    pdmlprotocols.h \
    pdmlreader.h

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
    lacpconfig.h \
    arpconfig.h \
    ip4config.h \
    ip6config.h \
    ip4over4config.h \
    gmpconfig.h \
    icmpconfig.h \
    igmpconfig.h \
    mldconfig.h \
    tcpconfig.h \
    udpconfig.h \
    textprotoconfig.h \
    hexdumpconfig.h \
    payloadconfig.h \
    sampleconfig.h \
    userscriptconfig.h

SOURCES += \
    ostprotolib.cpp \
    abstractfileformat.cpp \
    fileformat.cpp \
    pcapfileformat.cpp \
    pdmlfileformat.cpp \
    pdmlprotocol.cpp \
    pdmlprotocols.cpp \
    pdmlreader.cpp \

SOURCES += \
    protocolwidgetfactory.cpp \
    macconfig.cpp \
    vlanconfig.cpp \
    eth2config.cpp \
    dot3config.cpp \
    llcconfig.cpp \
    snapconfig.cpp \
    lacpconfig.cpp \
    arpconfig.cpp \
    ip4config.cpp \
    ip6config.cpp \
    gmpconfig.cpp \
    icmpconfig.cpp \
    igmpconfig.cpp \
    mldconfig.cpp \
    tcpconfig.cpp \
    udpconfig.cpp \
    textprotoconfig.cpp \
    hexdumpconfig.cpp \
    payloadconfig.cpp \
    sampleconfig.cpp \
    userscriptconfig.cpp

SOURCES += \
    vlanpdml.cpp \
    svlanpdml.cpp \
    lacppdml.cpp \
    eth2pdml.cpp \
    llcpdml.cpp \
    arppdml.cpp \
    ip4pdml.cpp \
    ip6pdml.cpp \
    icmppdml.cpp \
    icmp6pdml.cpp \
    igmppdml.cpp \
    mldpdml.cpp \
    tcppdml.cpp \
    udppdml.cpp \
    textprotopdml.cpp \
    samplepdml.cpp

QMAKE_DISTCLEAN += object_script.*

include(../protobuf.pri)
