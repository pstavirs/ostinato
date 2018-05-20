TEMPLATE = lib
CONFIG += qt staticlib warn_on
QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 4.0.0

DEFINES += QHEXEDIT_EXPORTS

HEADERS = src/chunks.h\
          src/commands.h \
          src/qhexedit.h \

SOURCES = src/chunks.cpp \
          src/commands.cpp \
          src/qhexedit.cpp
