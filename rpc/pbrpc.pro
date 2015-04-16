TEMPLATE = lib
CONFIG += qt staticlib
QT += network
DEFINES += HAVE_REMOTE
LIBS += -lprotobuf
HEADERS += rpcserver.h rpcconn.h pbrpccontroller.h pbrpcchannel.h pbqtio.h
SOURCES += rpcserver.cpp rpcconn.cpp pbrpcchannel.cpp
