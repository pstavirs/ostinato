TEMPLATE = app
CONFIG += qt debug
QT += network script
DEFINES += HAVE_REMOTE WPCAP
INCLUDEPATH += "../rpc"
win32:LIBS += -lwpcap -lpacket
unix:LIBS += -lpcap
win32:LIBS += -L"../common/debug" -lostproto
unix:LIBS += -L"../common" -lostproto
win32:LIBS += -L"../rpc/debug" -lpbrpc
unix:LIBS += -L"../rpc" -lpbrpc
LIBS += -lprotobuf
win32:POST_TARGETDEPS += "../common/debug/libostproto.a" "../rpc/debug/libpbrpc.a"
unix:POST_TARGETDEPS += "../common/libostproto.a" "../rpc/libpbrpc.a"
RESOURCES += drone.qrc 
HEADERS += drone.h 
FORMS += drone.ui
SOURCES += \
    drone_main.cpp \
    drone.cpp \
    portmanager.cpp \
    abstractport.cpp \
    pcapport.cpp \
    winpcapport.cpp 
SOURCES += myservice.cpp 
SOURCES += pcapextra.cpp 

