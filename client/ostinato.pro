TEMPLATE = app
CONFIG += qt debug
QT += network
RESOURCES += ostinato.qrc 
HEADERS += \
	hexlineedit.h \
	mainwindow.h \
	mythread.h \
	port.h \
	portgroup.h \
	portgrouplist.h \
	portmodel.h \
	portstatsmodel.h \
	portstatswindow.h \
	portswindow.h \
	streamconfigdialog.h \
	streammodel.h 

FORMS += \
	mainwindow.ui \
	portstatswindow.ui \
	portswindow.ui \
	streamconfigdialog.ui 

SOURCES += \
	stream.cpp \
	hexlineedit.cpp \
	main.cpp \
	mainwindow.cpp \
	mythread.cpp \
	port.cpp \
	portgroup.cpp \
	portgrouplist.cpp \
	portmodel.cpp \
	portstatsmodel.cpp \
	portstatswindow.cpp \
	portswindow.cpp \
	streamconfigdialog.cpp \
	streammodel.cpp 

# TODO(LOW): Test only
include(modeltest.pri)
