TEMPLATE = app
CONFIG += qt
QT += network
DEFINES += HAVE_REMOTE WPCAP
INCLUDEPATH += "c:\msys\1.0\local\include"
INCLUDEPATH += "C:\DevelLibs\WpdPack\Include"
INCLUDEPATH += "..\rpc"
LIBS += -L"C:\DevelLibs\WpdPack\Lib" -lwpcap
LIBS += -L"..\rpc\debug" -lpbrpc
HEADERS += drone.h 
FORMS += drone.ui
SOURCES += drone_main.cpp drone.cpp 
SOURCES += myservice.cpp 

SOURCES += "..\common\protocol.pb.cc"
