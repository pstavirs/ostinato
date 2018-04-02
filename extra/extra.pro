TEMPLATE = subdirs
SUBDIRS = \
    qhexedit2

greaterThan(QT_MINOR_VERSION, 6) {
SUBDIRS += modeltest
}
