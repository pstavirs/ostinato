TEMPLATE = lib
CONFIG += qt staticlib
QT += network script
LIBS += \
    -lprotobuf
FORMS += \
    mac.ui \
    payload.ui \
    eth2.ui \
    dot3.ui \
    llc.ui \
    snap.ui \
    vlan.ui \
    arp.ui \
    ip4.ui \
    ip6.ui \
    icmp.ui \
    tcp.ui \
    udp.ui \
    textproto.ui \
    userscript.ui \
    sample.ui 
PROTOS += \
    protocol.proto \
    mac.proto \
    payload.proto \
    eth2.proto \
    dot3.proto \
    llc.proto \
    snap.proto \
    dot2llc.proto \
    dot2snap.proto \
    vlan.proto \
    svlan.proto \
    vlanstack.proto \
    arp.proto \
    ip4.proto \
    ip6.proto \
    ip4over4.proto \
    icmp.proto \
    tcp.proto \
    udp.proto \
    textproto.proto \
    userscript.proto \
    sample.proto
HEADERS += \
    abstractprotocol.h    \
    comboprotocol.h    \
    protocolmanager.h \
    protocollist.h \
    protocollistiterator.h \
    streambase.h \
    mac.h \
    payload.h \
    eth2.h \
    dot3.h \
    llc.h \
    snap.h \
    dot2llc.h \
    dot2snap.h \
    vlan.h \
    svlan.h \
    vlanstack.h \
    arp.h \
    ip4.h \
    ip6.h \
    ip4over4.h \
    icmp.h \
    tcp.h \
    udp.h \
    textproto.h \
    userscript.h \
    sample.h
SOURCES += \
    abstractprotocol.cpp \
    protocolmanager.cpp \
    protocollist.cpp \
    protocollistiterator.cpp \
    streambase.cpp \
    mac.cpp \
    payload.cpp \
    eth2.cpp \
    dot3.cpp \
    llc.cpp \
    snap.cpp \
    vlan.cpp \
    svlan.cpp \
    arp.cpp \
    ip4.cpp \
    ip6.cpp \
    icmp.cpp \
    tcp.cpp \
    udp.cpp \
    textproto.cpp \
    userscript.cpp \
    sample.cpp

protobuf_decl.name  = protobuf header
protobuf_decl.input = PROTOS
protobuf_decl.output  = ${QMAKE_FILE_BASE}.pb.h
protobuf_decl.commands = protoc --cpp_out="." ${QMAKE_FILE_NAME}
protobuf_decl.variable_out = GENERATED_FILES 
QMAKE_EXTRA_COMPILERS += protobuf_decl 

protobuf_impl.name  = protobuf implementation
protobuf_impl.input = PROTOS
protobuf_impl.output  = ${QMAKE_FILE_BASE}.pb.cc
protobuf_impl.depends  = ${QMAKE_FILE_BASE}.pb.h
protobuf_impl.commands = $$escape_expand(\n)
protobuf_impl.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += protobuf_impl 

QMAKE_DISTCLEAN += object_script.*
