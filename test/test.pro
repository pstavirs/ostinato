TEMPLATE = app
CONFIG += qt console
QT += xml network script
INCLUDEPATH += "../rpc/" "../common/" "../client"

OBJDIR = .
win32 {
    LIBS += -lwpcap -lpacket
    CONFIG(debug, debug|release) {
        OBJDIR = debug
    } else {
        OBJDIR = release
    }
} else {
    LIBS += -lpcap
}
LIBS += -L"../common/$$OBJDIR" -lostfile -lostproto
LIBS += -L"../rpc/$$OBJDIR" -lpbrpc
POST_TARGETDEPS += \
    "../common/$$OBJDIR/libostfile.a" \
    "../common/$$OBJDIR/libostproto.a" \
    "../rpc/$$OBJDIR/libpbrpc.a"

LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2

HEADERS += 
SOURCES += main.cpp

QMAKE_DISTCLEAN += object_script.*

include(../install.pri)
