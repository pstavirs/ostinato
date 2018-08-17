TEMPLATE = app
CONFIG += qt ver_info
macx: TARGET = Ostinato
win32:RC_FILE = ostinato.rc
macx:ICON = icons/logo.icns
QT += widgets network script xml
INCLUDEPATH += "../rpc/" "../common/"
win32 {
    QMAKE_LFLAGS += -static
    CONFIG(debug, debug|release) {
        LIBS += -L"../common/debug" -lostprotogui -lostproto
        LIBS += -L"../rpc/debug" -lpbrpc
        POST_TARGETDEPS += \
            "../common/debug/libostprotogui.a" \
            "../common/debug/libostproto.a" \
            "../rpc/debug/libpbrpc.a"
    } else {
        LIBS += -L"../common/release" -lostprotogui -lostproto
        LIBS += -L"../rpc/release" -lpbrpc
        POST_TARGETDEPS += \
            "../common/release/libostprotogui.a" \
            "../common/release/libostproto.a" \
            "../rpc/release/libpbrpc.a"
    }
} else {
    LIBS += -L"../common" -lostprotogui -lostproto
    LIBS += -L"../rpc" -lpbrpc
    POST_TARGETDEPS += \
        "../common/libostprotogui.a" \
        "../common/libostproto.a" \
        "../rpc/libpbrpc.a"
}
LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2
RESOURCES += ostinato.qrc 
HEADERS += \
    arpstatusmodel.h \
    devicegroupdialog.h \
    devicegroupmodel.h \
    devicemodel.h \
    deviceswidget.h \
    dumpview.h \
    hexlineedit.h \
    logsmodel.h \
    logswindow.h \
    mainwindow.h \
    ndpstatusmodel.h \
    packetmodel.h \
    port.h \
    portconfigdialog.h \
    portgroup.h \
    portgrouplist.h \
    portmodel.h \
    portstatsfilterdialog.h \
    portstatsmodel.h \
    portstatsproxymodel.h \
    portstatswindow.h \
    portswindow.h \
    preferences.h \
    settings.h \
    streamconfigdialog.h \
    streamlistdelegate.h \
    streammodel.h \
    streamstatsfiltermodel.h \
    streamstatsmodel.h \
    streamstatswindow.h \
    variablefieldswidget.h

FORMS += \
    about.ui \
    devicegroupdialog.ui \
    deviceswidget.ui \
    logswindow.ui \
    mainwindow.ui \
    portconfigdialog.ui \
    portstatsfilter.ui \
    portstatswindow.ui \
    portswindow.ui \
    preferences.ui \
    streamconfigdialog.ui \
    streamstatswindow.ui \
    variablefieldswidget.ui

SOURCES += \
    arpstatusmodel.cpp \
    devicegroupdialog.cpp \
    devicegroupmodel.cpp \
    devicemodel.cpp \
    deviceswidget.cpp \
    dumpview.cpp \
    stream.cpp \
    hexlineedit.cpp \
    logsmodel.cpp \
    logswindow.cpp \
    main.cpp \
    mainwindow.cpp \
    ndpstatusmodel.cpp \
    packetmodel.cpp \
    params.cpp \
    port.cpp \
    portconfigdialog.cpp \
    portgroup.cpp \
    portgrouplist.cpp \
    portmodel.cpp \
    portstatsmodel.cpp \
    portstatsfilterdialog.cpp \
    portstatswindow.cpp \
    portswindow.cpp \
    preferences.cpp \
    streamconfigdialog.cpp \
    streamlistdelegate.cpp \
    streammodel.cpp \
    streamstatsmodel.cpp \
    streamstatswindow.cpp \
    variablefieldswidget.cpp


QMAKE_DISTCLEAN += object_script.*

include(../install.pri)
include(../version.pri)
include(../options.pri)

INCLUDEPATH += "../extra/modeltest"
greaterThan(QT_MINOR_VERSION, 6) {
CONFIG(debug, debug|release): LIBS += -L"../extra/modeltest/$(OBJECTS_DIR)/" -lmodeltest
CONFIG(debug, debug|release): QT += testlib
}
