TEMPLATE = app
CONFIG += qt console
QT += xml network script
INCLUDEPATH += "../rpc/" "../common/"
LIBS += -L"../common/debug" -lostproto
LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2
POST_TARGETDEPS +=  "../common/debug/libostproto.a" 

HEADERS += 
SOURCES += main.cpp

QMAKE_DISTCLEAN += object_script.*

include(../install.pri)
