TEMPLATE = lib
CONFIG += qt staticlib
QT += network
DEFINES += HAVE_REMOTE
INCLUDEPATH += "c:\msys\1.0\local\include"
LIBS += -L"C:\msys\1.0\local\lib" -lprotobuf
HEADERS += rpcserver.h pbrpccontroller.h pbrpcchannel.h
SOURCES += rpcserver.cpp pbrpcchannel.cpp
