TEMPLATE = app
CONFIG += qt ver_info c++11
addon: CONFIG -= ver_info
QT += network script xml
QT -= gui
linux*:system(grep -q IFLA_STATS64 /usr/include/linux/if_link.h): \
    DEFINES += HAVE_IFLA_STATS64
INCLUDEPATH += "../common"
INCLUDEPATH += "../rpc"

OBJDIR = .
win32 {
    # Support Windows Vista and above only
    DEFINES += WIN32_LEAN_AND_MEAN NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
    DEFINES += HAVE_REMOTE WPCAP
    CONFIG += console
    QMAKE_LFLAGS += -static
    LIBS += -lwpcap -lpacket -liphlpapi
    CONFIG(debug, debug|release) {
        OBJDIR = debug
    } else {
        OBJDIR = release
    }
} else {
    LIBS += -lpcap
}
LIBS += -L"../common/$$OBJDIR" -lostproto
LIBS += -L"../rpc/$$OBJDIR" -lpbrpc
POST_TARGETDEPS += \
    "../common/$$OBJDIR//libostproto.a" \
    "../rpc/$$OBJDIR/libpbrpc.a"

linux {
    INCLUDEPATH += "/usr/include/libnl3"
    LIBS += -lnl-3 -lnl-route-3
}
LIBS += -lm
LIBS += -lprotobuf
HEADERS += drone.h \
    pcaptransmitter.h \
    myservice.h
SOURCES += \
    devicemanager.cpp \
    device.cpp \
    emuldevice.cpp \
    drone_main.cpp \
    drone.cpp \
    portmanager.cpp \
    abstractport.cpp \
    pcapport.cpp \
    pcapsession.cpp \
    pcaptransmitter.cpp \
    pcaprxstats.cpp \
    pcaptxstats.cpp \
    pcaptxthread.cpp \
    bsdhostdevice.cpp \
    bsdport.cpp \
    linuxhostdevice.cpp \
    linuxport.cpp \
    linuxutils.cpp \
    params.cpp \
    turbo.cpp \
    winhostdevice.cpp \
    winpcapport.cpp 
SOURCES += myservice.cpp 
SOURCES += pcapextra.cpp 
SOURCES += packetbuffer.cpp

QMAKE_DISTCLEAN += object_script.*

include (../install.pri)
include (../version.pri)
include (../options.pri)
