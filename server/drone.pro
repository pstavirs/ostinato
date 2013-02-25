TEMPLATE = app
CONFIG += qt 
QT += network script
DEFINES += HAVE_REMOTE WPCAP
INCLUDEPATH += "../rpc"
win32 {
    LIBS += -lwpcap -lpacket
    CONFIG(debug, debug|release) {
        LIBS += -L"../common/debug" -lostproto
        LIBS += -L"../rpc/debug" -lpbrpc
        POST_TARGETDEPS += \
            "../common/debug/libostproto.a" \
            "../rpc/debug/libpbrpc.a"
    } else {
        LIBS += -L"../common/release" -lostproto
        LIBS += -L"../rpc/release" -lpbrpc
        POST_TARGETDEPS += \
            "../common/release/libostproto.a" \
            "../rpc/release/libpbrpc.a"
    }
} else {
    LIBS += -lpcap
    LIBS += -L"../common" -lostproto
    LIBS += -L"../rpc" -lpbrpc
    POST_TARGETDEPS += "../common/libostproto.a" "../rpc/libpbrpc.a"
}
LIBS += -lm
LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2
RESOURCES += drone.qrc 
HEADERS += drone.h 
FORMS += drone.ui
SOURCES += \
    drone_main.cpp \
    drone.cpp \
    portmanager.cpp \
    abstractport.cpp \
    pcapport.cpp \
    bsdport.cpp \
    linuxport.cpp \
    winpcapport.cpp 
SOURCES += myservice.cpp 
SOURCES += pcapextra.cpp 

QMAKE_DISTCLEAN += object_script.*

include (../install.pri)
include (../version.pri)
