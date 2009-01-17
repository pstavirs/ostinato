TEMPLATE = app
CONFIG += qt debug
QT += network
DEFINES += HAVE_REMOTE WPCAP
INCLUDEPATH += "../rpc"
win32:LIBS += -lwpcap -lpacket
unix:LIBS += -lpcap
win32:LIBS += -L"../rpc/debug" -lpbrpc
unix:LIBS += -L"../rpc" -lpbrpc
HEADERS += drone.h 
FORMS += drone.ui
SOURCES += drone_main.cpp drone.cpp 
SOURCES += myservice.cpp 

unix:SOURCES += pcapextra.cpp 

SOURCES += "..\common\protocol.pb.cc"
