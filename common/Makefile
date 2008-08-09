RM=del
PROTOC=protoc

#-#-#

all: protocol.pb.cc

protocol.pb.cc: protocol.proto
	$(PROTOC) --cpp_out=. $<

clean:
	$(RM) *.pb.h *.pb.cc

distclean: clean
