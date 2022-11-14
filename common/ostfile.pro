TEMPLATE = lib
CONFIG += qt staticlib
QT += widgets network xml script
LIBS += \
    -lprotobuf

FORMS = \
    pcapfileimport.ui \

PROTOS = \
    fileformat.proto 

HEADERS = \
    ostprotolib.h \
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
    streamfileformat.h

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
    vlanpdml.cpp \
    svlanpdml.cpp \
    stppdml.cpp \
    eth2pdml.cpp \
    llcpdml.cpp \
    arppdml.cpp \
    ip4pdml.cpp \
    ip6pdml.cpp \
    grepdml.cpp \
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
