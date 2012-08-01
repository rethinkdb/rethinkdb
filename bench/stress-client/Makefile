.PHONY: build
LIBS = -lpthread -ldl -lprotobuf
# Do not include main.cc or python_interface.cc in SRC
SRC = utils.cc random.cc protocol.cc protocols/sqlite3.c protocols/query_language.pb.cc
HEADERS = $(wildcard *.hpp) $(wildcard */*.hpp)

MYSQL ?= 1
LIBMEMCACHED ?= 0
LIBGSL ?= 0
TAGS=.tags

ifeq ($(MYSQL),1)
    INCLUDE += -I /usr/local/mysql/include
    LIBS += -lmysqlclient -L /usr/local/mysql/lib/mysql 
    DEFINES += -DUSE_MYSQL
endif

ifeq ($(LIBMEMCACHED),1)
    INCLUDE += -I../../lib/libmemcached
    LIBS += -lmemcached -L /usr/local/lib
    DEFINES += -DUSE_LIBMEMCACHED
endif

ifeq ($(LIBGSL),1)
    LIBS += -lgsl -lgslcblas
    DEFINES += -DUSE_LIBGSL
endif

EXEC_NAME = stress
SO_NAME = libstress.so
CXX = g++
CFLAGS = -fPIC
CXXFLAGS = -g $(INCLUDE) -I . $(DEFINES) -fPIC # -O3

PROTO_DIR = ../../src/rdb_protocol

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(EXEC_NAME) $(SO_NAME)

proto_files:
	protoc -I $(PROTO_DIR) --cpp_out=protocols/ $(PROTO_DIR)/query_language.proto

%.o: %.cc $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.pb.o: %.pb.cc $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXEC_NAME): proto_files $(OBJ) main.o $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) main.o -lm -lrt $(TLIB) $(LIBS)

$(SO_NAME): $(OBJ) python_interface.o $(HEADERS) python_interface.h Makefile
	$(CXX) $(CXXFLAGS) -shared -o $(SO_NAME) $(OBJ) python_interface.o -lm -lrt $(TLIB) $(LIBS)

tags: Makefile
	ctags -R -f $(TAGS) --langmap="c++:.cc.tcc.hpp"

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
	rm -f $(SO_NAME)
	rm -f protocols/query_language.pb.cc
	rm -f protocols/query_language.pb.h
