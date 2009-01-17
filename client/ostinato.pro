TEMPLATE = app
CONFIG += qt debug
QT += network
INCLUDEPATH += "../rpc/"
LIBS += -lprotobuf -L"../rpc/" -lpbrpc
RESOURCES += ostinato.qrc 
HEADERS += \
	dumpview.h \
	hexlineedit.h \
	mainwindow.h \
	packetmodel.h \
	port.h \
	portgroup.h \
	portgrouplist.h \
	portmodel.h \
	portstatsmodel.h \
	portstatsfilterdialog.h \
	portstatswindow.h \
	portswindow.h \
	streamconfigdialog.h \
	streammodel.h 

FORMS += \
	mainwindow.ui \
	portstatsfilter.ui \
	portstatswindow.ui \
	portswindow.ui \
	streamconfigdialog.ui 

SOURCES += \
	dumpview.cpp \
	stream.cpp \
	hexlineedit.cpp \
	main.cpp \
	mainwindow.cpp \
	packetmodel.cpp \
	port.cpp \
	portgroup.cpp \
	portgrouplist.cpp \
	portmodel.cpp \
	portstatsmodel.cpp \
	portstatsfilterdialog.cpp \
	portstatswindow.cpp \
	portswindow.cpp \
	streamconfigdialog.cpp \
	streammodel.cpp 

# Protocol Buffer Sources

SOURCES += \
	..\common\protocol.pb.cc

# TODO(LOW): Test only
include(modeltest.pri)
