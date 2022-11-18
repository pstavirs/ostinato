TEMPLATE = app
CONFIG += qt ver_info
macx: TARGET = Ostinato
win32:RC_FILE = ostinato.rc
macx:ICON = icons/logo.icns
QT += widgets network script xml svg
INCLUDEPATH += "../rpc/" "../common/"

OBJDIR = .
win32 {
    QMAKE_LFLAGS += -static
    CONFIG(debug, debug|release) {
        OBJDIR = debug
    } else {
        OBJDIR = release
    }
}
LIBS += -L"../common/$$OBJDIR" -lostfile -lostfilegui
LIBS += -L"../common/$$OBJDIR" -lostprotogui -lostproto
LIBS += -L"../rpc/$$OBJDIR" -lpbrpc
POST_TARGETDEPS += \
    "../common/$$OBJDIR/libostfilegui.a" \
    "../common/$$OBJDIR/libostfile.a" \
    "../common/$$OBJDIR/libostprotogui.a" \
    "../common/$$OBJDIR/libostproto.a" \
    "../rpc/$$OBJDIR/libpbrpc.a"

LIBS += -lprotobuf
LIBS += -L"../extra/qhexedit2/$(OBJECTS_DIR)/" -lqhexedit2
RESOURCES += ostinato.qrc 
HEADERS += \
    arpstatusmodel.h \
    clipboardhelper.h \
    devicegroupdialog.h \
    devicegroupmodel.h \
    devicemodel.h \
    deviceswidget.h \
    dumpview.h \
    fieldedit.h \
    hexlineedit.h \
    logsmodel.h \
    logswindow.h \
    findreplace.h \
    mainwindow.h \
    mandatoryfieldsgroup.h \
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
    portwidget.h \
    preferences.h \
    settings.h \
    streamconfigdialog.h \
    streamlistdelegate.h \
    streammodel.h \
    streamstatsfiltermodel.h \
    streamstatsmodel.h \
    streamstatswindow.h \
    streamswidget.h \
    variablefieldswidget.h \
    xtableview.h

FORMS += \
    about.ui \
    devicegroupdialog.ui \
    deviceswidget.ui \
    findreplace.ui \
    logswindow.ui \
    mainwindow.ui \
    portconfigdialog.ui \
    portstatsfilter.ui \
    portstatswindow.ui \
    portswindow.ui \
    portwidget.ui \
    preferences.ui \
    streamconfigdialog.ui \
    streamstatswindow.ui \
    streamswidget.ui \
    variablefieldswidget.ui

SOURCES += \
    arpstatusmodel.cpp \
    clipboardhelper.cpp \
    devicegroupdialog.cpp \
    devicegroupmodel.cpp \
    devicemodel.cpp \
    deviceswidget.cpp \
    dumpview.cpp \
    stream.cpp \
    hexlineedit.cpp \
    logsmodel.cpp \
    logswindow.cpp \
    fieldedit.cpp \
    findreplace.cpp \
    main.cpp \
    mainwindow.cpp \
    mandatoryfieldsgroup.cpp \
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
    portwidget.cpp \
    preferences.cpp \
    streamconfigdialog.cpp \
    streamlistdelegate.cpp \
    streammodel.cpp \
    streamstatsmodel.cpp \
    streamstatswindow.cpp \
    streamswidget.cpp \
    thememanager.cpp \
    variablefieldswidget.cpp

THEMES += \
    themes/material-dark.qss \
    themes/material-dark.rcc \
    themes/material-light.qss \
    themes/material-light.rcc \
    themes/qds-dark.qss \
    themes/qds-dark.rcc \
    themes/qds-light.qss \
    themes/qds-light.rcc \

QMAKE_DISTCLEAN += object_script.*

include(../install.pri)
include(../shared.pri)
include(../version.pri)
include(../options.pri)

INCLUDEPATH += "../extra/modeltest"
greaterThan(QT_MINOR_VERSION, 6) {
CONFIG(debug, debug|release): LIBS += -L"../extra/modeltest/$(OBJECTS_DIR)/" -lmodeltest
CONFIG(debug, debug|release): QT += testlib
}
