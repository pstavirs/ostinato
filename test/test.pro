TEMPLATE = app
CONFIG += qt console
QT += xml network script widgets
INCLUDEPATH += "../rpc/" "../common/" "../client"
win32 {
    LIBS += -lwpcap -lpacket
    CONFIG(debug, debug|release) {
        LIBS += -L"../common/debug" -lostfile -lostproto
        LIBS += -L"../rpc/debug" -lpbrpc
        POST_TARGETDEPS += \
            "../common/debug/libostfile.a" \
            "../common/debug/libostproto.a" \
            "../rpc/debug/libpbrpc.a"
    } else {
        LIBS += -L"../common/release" -lostfile -lostproto
        LIBS += -L"../rpc/release" -lpbrpc
        POST_TARGETDEPS += \
            "../common/release/libostfile.a" \
            "../common/release/libostproto.a" \
            "../rpc/release/libpbrpc.a"
    }
} else {
    LIBS += -lpcap
    LIBS += -L"../common" -lostfile -lostproto
    LIBS += -L"../rpc" -lpbrpc
    POST_TARGETDEPS += \
        "../common/libostfile.a" \
        "../common/libostproto.a" \
        "../rpc/libpbrpc.a" 
}
LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2

HEADERS += 
SOURCES += main.cpp

QMAKE_DISTCLEAN += object_script.*

include(../install.pri)
