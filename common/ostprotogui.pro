TEMPLATE = lib
CONFIG += qt staticlib
QT += widgets network xml script
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
    stp.ui \
    arp.ui \
    ip4.ui \
    ip6.ui \
    gmp.ui \
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

PROTOS = \
    fileformat.proto 

# TODO: Move fileformat related stuff into a different library - why?
HEADERS = \
    ostprotolib.h \
    ipv4addressdelegate.h \
    ipv6addressdelegate.h \
    nativefileformat.h \
    ossnfileformat.h \
    ostmfileformat.h \
    pcapfileformat.h \
    pdmlfileformat.h \
    pythonfileformat.h \
    pdmlprotocol.h \
    pdmlprotocols.h \
    pdmlreader.h \
    sessionfileformat.h \
    streamfileformat.h \
    spinboxdelegate.h

HEADERS += \
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
    ostprotolib.cpp \
    nativefileformat.cpp \
    ossnfileformat.cpp \
    ostmfileformat.cpp \
    pcapfileformat.cpp \
    pdmlfileformat.cpp \
    pythonfileformat.cpp \
    pdmlprotocol.cpp \
    pdmlprotocols.cpp \
    pdmlreader.cpp \
    sessionfileformat.cpp \
    streamfileformat.cpp \
    spinboxdelegate.cpp

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

SOURCES += \
    vlanpdml.cpp \
    svlanpdml.cpp \
    stppdml.cpp \
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
include(../options.pri)
