#
# Qt qmake integration with Google Protocol Buffers compiler protoc
# Author: Srivats P.
# 
# To compile protocol buffers with qt qmake, specify PROTOS variable and
# include this file
#
# Example:
# PROTOS = a.proto b.proto
# include(protobuf.pri)
#
# By default protoc looks for .proto files (including the imported ones) in
# the current directory where protoc is run. If you need to include additional
# paths specify the PROTOPATH variable
#

PROTOPATH += .
PROTOPATHS = 
for(p, PROTOPATH):PROTOPATHS += --proto_path=$${p}

PROTO_CC += $$replace(PROTOS, \.proto, .pb.cc)

protobuf_decl.name  = protobuf header
protobuf_decl.input = PROTOS
protobuf_decl.output  = ${QMAKE_FILE_BASE}.pb.h
protobuf_decl.commands = protoc --cpp_out="." $${PROTOPATHS} ${QMAKE_FILE_NAME}
protobuf_decl.variable_out = GENERATED_FILES 
QMAKE_EXTRA_COMPILERS += protobuf_decl 

protobuf_impl.name  = protobuf implementation
protobuf_impl.input = PROTOS
protobuf_impl.output  = ${QMAKE_FILE_BASE}.pb.cc
protobuf_impl.depends  = ${QMAKE_FILE_BASE}.pb.h
protobuf_impl.commands = $$escape_expand(\\n)
protobuf_impl.variable_out = GENERATED_FILES
QMAKE_EXTRA_COMPILERS += protobuf_impl 

# protobuf generated code emits compiler warnings, so use -Wno-error to
# compile 'em; if and when protobuf generates clean code, make the following
# changes -
# - protobuf_impl.variable_out = GENERATED_FILES
# + protobuf_impl.variable_out = GENERATED_SOURCES
# - QMAKE_EXTRA_COMPILERS += protobuf_cc
# - #QMAKE_EXTRA_COMPILERS += protobuf_cc
protobuf_cc.name = protobuf cc compilation
protobuf_cc.input = PROTO_CC
protobuf_cc.output = ${QMAKE_FILE_BASE}.o
protobuf_cc.dependency_type = TYPE_C
protobuf_cc.commands = $(CXX) -c $(CXXFLAGS) -Wno-error $(INCPATH) -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
protobuf_cc.variable_out = OBJECTS
QMAKE_EXTRA_COMPILERS += protobuf_cc
