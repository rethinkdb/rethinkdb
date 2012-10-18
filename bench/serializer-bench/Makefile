DEBUG?=0
CXXFLAGS=-Wall -pg -g -DUSE_UCONTEXT -DNDEBUG=1
LDFLAGS=-Wall -rdynamic -lrt -laio -pg -g -lgsl -lgslcblas -pthread -lv8 -lcrypto
OBJDIR:=../../build/release/obj
#STATIC_LIBRARIES:=boost_serialization protobuf boost_program_options
STATIC_LIBRARIES:=protobuf boost_program_options
EXTERNAL_SOURCE_DIR:=/usr/src/rethinkdb_lib_external

# look for the static library in the same directory as the .so file
STATIC_LIBRARY_PATHS:=$(foreach lib,$(STATIC_LIBRARIES),$(shell /sbin/ldconfig -p | awk '/lib$(lib).so / { gsub("\\.so$$", ".a", $$NF); print $$NF; exit 0; }')) $(EXTERNAL_SOURCE_DIR)/WebKit/WebKitBuild/Release/JavaScriptCore/release/libjscore.a

serializer-bench: main.cc Makefile
	cd ../../src && make DEBUG=0 -j8
	g++ main.cc -I ../../src/ -c -o main.o $(CXXFLAGS)
	g++ main.o `find $(OBJDIR) -name "*.o" | grep -v main.o | grep -v 'unittest/'` $(STATIC_LIBRARY_PATHS) -o serializer-bench $(LDFLAGS)

clean:
	rm -f *~
	rm -f *.o
	rm -f serializer-bench
