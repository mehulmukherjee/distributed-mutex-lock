LDFLAGS = -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
	   -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11 -I . -g

GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOC = protoc

DMLDIR = dml-test
SQCDIR = seq-consistency


#generate all executable (takes a long time)
all: dml dsm
	
#generate executable for DML tests	
dml: dml_p1

dml_p1: psu_dml.pb.o psu_dml.grpc.pb.o $(DMLDIR)/p1.o
	$(CXX) $^ $(LDFLAGS) -o $@

#generate executables for sequential consistency tests
dsm: dsm_p1 dsm_p2 dsm_p3 dsm_server

dsm_p1: psu_dsm.pb.o psu_dsm.grpc.pb.o $(SQCDIR)/p1.o
	$(CXX) $^ $(LDFLAGS) -o $@
dsm_p2: psu_dsm.pb.o psu_dsm.grpc.pb.o $(SQCDIR)/p2.o
	$(CXX) $^ $(LDFLAGS) -o $@
dsm_p3: psu_dsm.pb.o psu_dsm.grpc.pb.o $(SQCDIR)/p3.o
	$(CXX) $^ $(LDFLAGS) -o $@
dsm_server: psu_dsm.pb.o psu_dsm.grpc.pb.o psu_dsm_directory.o
	$(CXX) $^ $(LDFLAGS) -o $@

#generate gRPC cc files for later compilation into obj files	
%.grpc.pb.cc: %.proto
	$(PROTOC) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) --cpp_out=. $<

#clean up my mess
fresh:
	rm -f *_p1 *_p2 *_p3 dsm_server $(DMLDIR)/*.o $(SQCDIR)/*.o psu_dsm_directory.o
clean:
	rm -f *.o *.pb.cc *.pb.h *_p1 *_p2 *_p3 dsm_server $(DMLDIR)/*.o $(SQCDIR)/*.o rpc-log-file.txt
